/* GPU ray tracing backend (Direct3D 11 compute) */

#include "gputrace.h"

#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <stdio.h>
#include <string.h>
#include <string>

#pragma comment( lib, "d3d11.lib" )
#pragma comment( lib, "d3dcompiler.lib" )

#define GPU_MAX_RAYS_PER_DISPATCH   ( 1 << 18 )
#define GPU_MIN_DIRECTIONS          16
#define GPU_MAX_JOBS_PER_DISPATCH   ( GPU_MAX_RAYS_PER_DISPATCH / GPU_MIN_DIRECTIONS )
#define GPU_GROUP_SIZE              64


/* The traversal mirrors Geo_TraceNode_r / Geo_TraceTriangle on the CPU.  Because
   radiosity rays carry no supersampled alpha, a CPU trace can only ever keep one
   hit: the nearest solid one.  That is all this shader computes. */
static const char gpuShaderSource[] =
R"HLSL(
struct Node  { float3 n; float d; int c0; int c1; };
struct Leaf  { int count; int first; };
struct Tri   { float3 v0; float nx; float3 v1; float ny; float3 v2; float nz;
               float2 st0; float2 st1; float2 st2; int mtl; int pad; };
struct Mtl   { uint offset; uint width; uint height; uint pad; };
struct Job   { float3 pos; uint seed; float3 axis0; float p0; float3 axis1; float p1; float3 axis2; float p2; };
struct Hit   { int tri; float u; float v; float frac; };

StructuredBuffer<Node>  gNodes   : register( t0 );
StructuredBuffer<Leaf>  gLeafs   : register( t1 );
StructuredBuffer<int>   gTriRefs : register( t2 );
StructuredBuffer<Tri>   gTris    : register( t3 );
StructuredBuffer<Mtl>   gMtls    : register( t4 );
StructuredBuffer<uint>  gMasks   : register( t5 );
StructuredBuffer<Job>   gJobs    : register( t6 );
StructuredBuffer<float4> gDirs   : register( t7 );     // x, y, jitter radius
RWStructuredBuffer<Hit> gHits    : register( u0 );

cbuffer Params : register( b0 ) { uint gRayCount; uint gTraceCount; uint gPad1; uint gPad2; };

uint Pcg( uint v )
{
    uint s = v * 747796405u + 2891336453u;
    uint w = ( ( s >> ( ( s >> 28u ) + 4u ) ) ^ s ) * 277803737u;
    return ( w >> 22u ) ^ w;
}

#define STACK_SIZE 96

bool MaskSolid( Mtl m, float2 st )
{
    int x = (int)round( (float)m.width * st.x );
    int y = (int)round( st.y * (float)m.height );
    x &= (int)m.width - 1;
    y &= (int)m.height - 1;
    uint bit = (uint)y * m.width + (uint)x;
    return ( gMasks[m.offset + ( bit >> 5 )] & ( 1u << ( bit & 31 ) ) ) != 0;
}

void TraceTri( int triIndex, float3 start, float3 delta,
               inout float bestFrac, inout int bestTri, inout float bestU, inout float bestV )
{
    Tri t = gTris[triIndex];
    float3 n = float3( t.nx, t.ny, t.nz );

    float denom = dot( delta, n );
    float sgn = 1.0;
    if ( denom >= 0.0 ) { denom = -denom; sgn = -1.0; }

    float3 e1 = t.v0 - start;
    float dist = dot( e1, n ) * sgn;
    if ( dist > 0.0 ) return;
    if ( !( denom * bestFrac < dist ) ) return;

    float3 cr = cross( delta, e1 );
    float3 ea = t.v0 - t.v1;
    float edgeDot1 = dot( cr, ea ) * sgn;
    if ( 0.0 < edgeDot1 ) return;

    float3 eb = t.v0 - t.v2;
    float edgeDot2 = sgn * dot( cr, eb );
    if ( 0.0 > edgeDot2 ) return;

    float3 c2 = cross( eb, ea );
    float edgeDot3 = dot( delta, c2 ) * sgn;
    if ( edgeDot3 > edgeDot1 - edgeDot2 ) return;

    float u = -edgeDot2 / edgeDot3;
    float v = edgeDot1 / edgeDot3;

    if ( t.mtl >= 0 )
    {
        float2 st = t.st0 + ( t.st1 - t.st0 ) * u + ( t.st2 - t.st0 ) * v;
        if ( !MaskSolid( gMtls[t.mtl], st ) ) return;
    }

    float frac = dist / denom;
    if ( frac < bestFrac )
    {
        bestFrac = frac; bestTri = triIndex; bestU = u; bestV = v;
    }
}

struct Frame { int child; float t0; float t1; };

[numthreads( 64, 1, 1 )]
void main( uint3 id : SV_DispatchThreadID )
{
    if ( id.x >= gRayCount ) return;

    // Build this ray the way Compile_RadiosityDir does on the CPU
    uint jobIndex = id.x / gTraceCount;
    uint dirIndex = id.x - jobIndex * gTraceCount;
    Job job = gJobs[jobIndex];
    float4 sampleDir = gDirs[dirIndex];

    uint rng = Pcg( job.seed * 2654435769u + dirIndex * 2246822519u + 1750411684u );
    float u = 0.0;
    float v = 0.0;
    float lenSq = 2.0;

    [loop] for ( int tries = 0; tries < 32 && lenSq > 1.0; tries++ )
    {
        rng = Pcg( rng );
        u = (float)( rng >> 8 ) * ( 2.0 / 16777216.0 ) - 1.0;
        rng = Pcg( rng );
        v = (float)( rng >> 8 ) * ( 2.0 / 16777216.0 ) - 1.0;
        lenSq = u * u + v * v;
    }

    if ( lenSq > 1.0 ) { u = 0.0; v = 0.0; }

    float lx = u * sampleDir.z + sampleDir.x;
    float ly = v * sampleDir.z + sampleDir.y;
    float lz;
    lenSq = lx * lx + ly * ly;

    if ( lenSq > 1.0 )
    {
        float inv = 1.0 / sqrt( lenSq );
        lx *= inv;
        ly *= inv;
        lz = 0.001;
    }
    else
    {
        lz = sqrt( 1.0 - lenSq );
    }

    float3 dir = job.axis0 * lx + job.axis1 * ly + job.axis2 * lz;
    float3 start = job.pos + dir * 0.125;
    float3 end = start + dir * 262144.0;
    float3 delta = end - start;

    float bestFrac = 1.0;
    int bestTri = -1;
    float bestU = 0.0;
    float bestV = 0.0;

    Frame stack[STACK_SIZE];
    int sp = 0;
    int child = 0;
    float t0 = 0.0;
    float t1 = 1.0;

    [loop] for ( ;; )
    {
        [loop] while ( child >= 0 )
        {
            Node node = gNodes[child];
            float d0 = dot( start, node.n ) - node.d;
            float d1 = dot( end, node.n ) - node.d;

            if ( d0 - d1 == 0.0 )
            {
                child = d0 > 0.0 ? node.c0 : node.c1;
                continue;
            }

            float frac = d0 / ( d0 - d1 );

            if ( frac < t0 )
            {
                child = d1 > 0.0 ? node.c0 : node.c1;
                continue;
            }

            int nearChild = d0 < 0.0 ? node.c1 : node.c0;
            int farChild  = d0 < 0.0 ? node.c0 : node.c1;

            if ( t1 < frac )
            {
                child = nearChild;
                continue;
            }

            if ( sp < STACK_SIZE )
            {
                stack[sp].child = farChild;
                stack[sp].t0 = frac;
                stack[sp].t1 = t1;
                sp++;
            }
            child = nearChild;
            t1 = frac;
        }

        if ( child != -1 )
        {
            Leaf leaf = gLeafs[-2 - child];
            for ( int i = 0; i < leaf.count; i++ )
                TraceTri( gTriRefs[leaf.first + i], start, delta, bestFrac, bestTri, bestU, bestV );
        }

        bool found = false;
        [loop] while ( sp > 0 )
        {
            sp--;
            Frame f = stack[sp];
            if ( f.t0 >= bestFrac ) continue;
            child = f.child; t0 = f.t0; t1 = f.t1;
            found = true;
            break;
        }
        if ( !found ) break;
    }

    Hit h;
    h.tri = bestTri;
    if ( bestTri >= 0 )
    {
        Tri hitTri = gTris[bestTri];
        float facing = dot( float3( hitTri.nx, hitTri.ny, hitTri.nz ), dir );
        if ( !( facing < 0.0 ) ) h.tri |= 0x10000000;
        if ( dir.z < 0.0 ) h.tri |= 0x20000000;
    }
    h.u = bestU;
    h.v = bestV;
    h.frac = bestFrac;
    gHits[id.x] = h;
}
)HLSL";


static struct
{
    ID3D11Device              *device;
    ID3D11DeviceContext       *context;
    ID3D11ComputeShader       *shader;

    ID3D11Buffer              *sceneBuffers[6];
    ID3D11ShaderResourceView  *sceneViews[6];

    ID3D11Buffer              *jobBuffer;
    ID3D11ShaderResourceView  *jobView;
    ID3D11Buffer              *dirBuffer;
    ID3D11ShaderResourceView  *dirView;
    int                        directionCount;
    ID3D11Buffer              *hitBuffer;
    ID3D11UnorderedAccessView *hitView;
    ID3D11Buffer              *hitStaging;
    ID3D11Buffer              *params;

    CRITICAL_SECTION           lock;
    int                        lockInit;

    double                     statRays;
    double                     statBusy;
    double                     statWait;
    char                       adapterName[128];
} gpu;


template <class T> static void SafeRelease( T *&obj )
{
    if ( obj )
    {
        obj->Release();
        obj = NULL;
    }
}

static void GpuError( char *err, int errSize, const char *what, HRESULT hr )
{
    _snprintf( err, errSize, "%s (HRESULT 0x%08x)", what, ( unsigned )hr );
    err[errSize - 1] = 0;
}

static HRESULT CreateStructured( const void *data, int elemSize, int elemCount,
                                 ID3D11Buffer **buffer, ID3D11ShaderResourceView **view )
{
    D3D11_BUFFER_DESC desc;
    D3D11_SUBRESOURCE_DATA init;
    std::string zeros;
    HRESULT hr;

    if ( elemCount < 1 )
    {
        elemCount = 1;
        data = NULL;
    }

    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth           = ( UINT )( elemSize * elemCount );
    desc.Usage               = D3D11_USAGE_DEFAULT;
    desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = ( UINT )elemSize;

    /* A buffer can't be empty, so give element-less scenes one zeroed record */
    if ( !data )
    {
        zeros.assign( desc.ByteWidth, '\0' );
        data = zeros.data();
    }

    init.pSysMem          = data;
    init.SysMemPitch      = 0;
    init.SysMemSlicePitch = 0;

    hr = gpu.device->CreateBuffer( &desc, &init, buffer );
    if ( FAILED( hr ) )
        return hr;

    return gpu.device->CreateShaderResourceView( *buffer, NULL, view );
}

int GpuTrace_Init( const GpuScene_t *scene, char *err, int errSize )
{
    D3D_FEATURE_LEVEL level;
    D3D_FEATURE_LEVEL wanted = D3D_FEATURE_LEVEL_11_0;
    ID3DBlob *code = NULL;
    ID3DBlob *messages = NULL;
    D3D11_BUFFER_DESC desc;
    HRESULT hr;
    IDXGIDevice *dxgiDevice = NULL;
    IDXGIAdapter *adapter = NULL;
    int i;

    memset( &gpu, 0, sizeof( gpu ) );

    hr = D3D11CreateDevice( NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, &wanted, 1,
                            D3D11_SDK_VERSION, &gpu.device, &level, &gpu.context );
    if ( FAILED( hr ) )
    {
        GpuError( err, errSize, "No Direct3D 11 hardware device", hr );
        return 0;
    }

    strcpy( gpu.adapterName, "unknown adapter" );

    if ( SUCCEEDED( gpu.device->QueryInterface( __uuidof( IDXGIDevice ), ( void ** )&dxgiDevice ) ) )
    {
        if ( SUCCEEDED( dxgiDevice->GetAdapter( &adapter ) ) )
        {
            DXGI_ADAPTER_DESC adapterDesc;

            if ( SUCCEEDED( adapter->GetDesc( &adapterDesc ) ) )
                WideCharToMultiByte( CP_ACP, 0, adapterDesc.Description, -1, gpu.adapterName,
                                     sizeof( gpu.adapterName ), NULL, NULL );
        }
        SafeRelease( adapter );
        SafeRelease( dxgiDevice );
    }

    hr = D3DCompile( gpuShaderSource, sizeof( gpuShaderSource ) - 1, "gputrace", NULL, NULL,
                     "main", "cs_5_0", D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code, &messages );
    if ( FAILED( hr ) )
    {
        if ( messages )
            _snprintf( err, errSize, "Shader compile failed: %s",
                       ( const char * )messages->GetBufferPointer() );
        else
            GpuError( err, errSize, "Shader compile failed", hr );
        err[errSize - 1] = 0;
        SafeRelease( messages );
        GpuTrace_Shutdown();
        return 0;
    }
    SafeRelease( messages );

    hr = gpu.device->CreateComputeShader( code->GetBufferPointer(), code->GetBufferSize(),
                                          NULL, &gpu.shader );
    SafeRelease( code );
    if ( FAILED( hr ) )
    {
        GpuError( err, errSize, "CreateComputeShader failed", hr );
        GpuTrace_Shutdown();
        return 0;
    }

    CreateStructured( scene->nodes,   sizeof( GpuNode_t ), scene->nodeCount,
                      &gpu.sceneBuffers[0], &gpu.sceneViews[0] );
    CreateStructured( scene->leafs,   sizeof( GpuLeaf_t ), scene->leafCount,
                      &gpu.sceneBuffers[1], &gpu.sceneViews[1] );
    CreateStructured( scene->triRefs, sizeof( int ),       scene->triRefCount,
                      &gpu.sceneBuffers[2], &gpu.sceneViews[2] );
    CreateStructured( scene->tris,    sizeof( GpuTri_t ),  scene->triCount,
                      &gpu.sceneBuffers[3], &gpu.sceneViews[3] );
    CreateStructured( scene->mtls,    sizeof( GpuMtl_t ),  scene->mtlCount,
                      &gpu.sceneBuffers[4], &gpu.sceneViews[4] );
    CreateStructured( scene->masks,   sizeof( unsigned ),  scene->maskWordCount,
                      &gpu.sceneBuffers[5], &gpu.sceneViews[5] );

    for ( i = 0; i < 6; i++ )
    {
        if ( !gpu.sceneBuffers[i] || !gpu.sceneViews[i] )
        {
            GpuError( err, errSize, "Couldn't upload the scene to the GPU", E_OUTOFMEMORY );
            GpuTrace_Shutdown();
            return 0;
        }
    }

    /* Job input (CPU write) */
    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth           = GPU_MAX_JOBS_PER_DISPATCH * sizeof( GpuJob_t );
    desc.Usage               = D3D11_USAGE_DYNAMIC;
    desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof( GpuJob_t );
    hr = gpu.device->CreateBuffer( &desc, NULL, &gpu.jobBuffer );
    if ( SUCCEEDED( hr ) )
        hr = gpu.device->CreateShaderResourceView( gpu.jobBuffer, NULL, &gpu.jobView );

    /* Hit output (GPU write) */
    if ( SUCCEEDED( hr ) )
    {
        memset( &desc, 0, sizeof( desc ) );
        desc.ByteWidth           = GPU_MAX_RAYS_PER_DISPATCH * sizeof( GpuHit_t );
        desc.Usage               = D3D11_USAGE_DEFAULT;
        desc.BindFlags           = D3D11_BIND_UNORDERED_ACCESS;
        desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
        desc.StructureByteStride = sizeof( GpuHit_t );
        hr = gpu.device->CreateBuffer( &desc, NULL, &gpu.hitBuffer );
    }

    if ( SUCCEEDED( hr ) )
        hr = gpu.device->CreateUnorderedAccessView( gpu.hitBuffer, NULL, &gpu.hitView );

    /* Readback */
    if ( SUCCEEDED( hr ) )
    {
        memset( &desc, 0, sizeof( desc ) );
        desc.ByteWidth      = GPU_MAX_RAYS_PER_DISPATCH * sizeof( GpuHit_t );
        desc.Usage          = D3D11_USAGE_STAGING;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        hr = gpu.device->CreateBuffer( &desc, NULL, &gpu.hitStaging );
    }

    if ( SUCCEEDED( hr ) )
    {
        memset( &desc, 0, sizeof( desc ) );
        desc.ByteWidth      = 16;
        desc.Usage          = D3D11_USAGE_DYNAMIC;
        desc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        hr = gpu.device->CreateBuffer( &desc, NULL, &gpu.params );
    }

    if ( FAILED( hr ) )
    {
        GpuError( err, errSize, "Couldn't allocate GPU buffers", hr );
        GpuTrace_Shutdown();
        return 0;
    }

    InitializeCriticalSection( &gpu.lock );
    gpu.lockInit = 1;

    return 1;
}

void GpuTrace_Shutdown( void )
{
    int i;

    SafeRelease( gpu.params );
    SafeRelease( gpu.hitStaging );
    SafeRelease( gpu.hitView );
    SafeRelease( gpu.hitBuffer );
    SafeRelease( gpu.dirView );
    SafeRelease( gpu.dirBuffer );
    SafeRelease( gpu.jobView );
    SafeRelease( gpu.jobBuffer );

    for ( i = 0; i < 6; i++ )
    {
        SafeRelease( gpu.sceneViews[i] );
        SafeRelease( gpu.sceneBuffers[i] );
    }

    SafeRelease( gpu.shader );
    SafeRelease( gpu.context );
    SafeRelease( gpu.device );

    if ( gpu.lockInit )
    {
        DeleteCriticalSection( &gpu.lock );
        gpu.lockInit = 0;
    }
}

const char *GpuTrace_AdapterName( void )
{
    return gpu.adapterName;
}

int GpuTrace_SetDirections( const float *dirX, const float *dirY, const float *jitter, int count )
{
    float *packed;
    int    i;
    HRESULT hr;

    if ( count < GPU_MIN_DIRECTIONS || count > 512 )
        return 0;

    packed = new float[count * 4];

    for ( i = 0; i < count; i++ )
    {
        packed[i * 4 + 0] = dirX[i];
        packed[i * 4 + 1] = dirY[i];
        packed[i * 4 + 2] = jitter[i];
        packed[i * 4 + 3] = 0.0f;
    }

    SafeRelease( gpu.dirView );
    SafeRelease( gpu.dirBuffer );

    hr = CreateStructured( packed, 16, count, &gpu.dirBuffer, &gpu.dirView );

    delete[] packed;

    if ( FAILED( hr ) )
        return 0;

    gpu.directionCount = count;

    return 1;
}

static int TraceSlice( const GpuJob_t *jobs, int jobCount, GpuHit_t *hits )
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    ID3D11ShaderResourceView *views[8];
    ID3D11ShaderResourceView *noViews[8] = { 0 };
    ID3D11UnorderedAccessView *uav = gpu.hitView;
    ID3D11UnorderedAccessView *noUav = NULL;
    ID3D11Buffer *cb = gpu.params;
    D3D11_BOX box;
    int rayCount = jobCount * gpu.directionCount;
    int i;

    if ( FAILED( gpu.context->Map( gpu.jobBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
        return 0;

    memcpy( mapped.pData, jobs, ( size_t )jobCount * sizeof( GpuJob_t ) );
    gpu.context->Unmap( gpu.jobBuffer, 0 );

    if ( SUCCEEDED( gpu.context->Map( gpu.params, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
    {
        unsigned *p = ( unsigned * )mapped.pData;

        p[0] = ( unsigned )rayCount;
        p[1] = ( unsigned )gpu.directionCount;
        p[2] = p[3] = 0;
        gpu.context->Unmap( gpu.params, 0 );
    }

    for ( i = 0; i < 6; i++ )
        views[i] = gpu.sceneViews[i];
    views[6] = gpu.jobView;
    views[7] = gpu.dirView;

    gpu.context->CSSetShader( gpu.shader, NULL, 0 );
    gpu.context->CSSetShaderResources( 0, 8, views );
    gpu.context->CSSetUnorderedAccessViews( 0, 1, &uav, NULL );
    gpu.context->CSSetConstantBuffers( 0, 1, &cb );

    gpu.context->Dispatch( ( UINT )( ( rayCount + GPU_GROUP_SIZE - 1 ) / GPU_GROUP_SIZE ), 1, 1 );

    gpu.context->CSSetShaderResources( 0, 8, noViews );
    gpu.context->CSSetUnorderedAccessViews( 0, 1, &noUav, NULL );

    box.left  = 0; box.right  = ( UINT )rayCount * sizeof( GpuHit_t );
    box.top   = 0; box.bottom = 1;
    box.front = 0; box.back   = 1;
    gpu.context->CopySubresourceRegion( gpu.hitStaging, 0, 0, 0, 0, gpu.hitBuffer, 0, &box );

    /* Map blocks until the dispatch has finished */
    if ( SUCCEEDED( gpu.context->Map( gpu.hitStaging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
    {
        memcpy( hits, mapped.pData, ( size_t )rayCount * sizeof( GpuHit_t ) );
        gpu.context->Unmap( gpu.hitStaging, 0 );
        return 1;
    }

    return 0;
}

double GpuTrace_Seconds( void )
{
    static double invFreq;
    LARGE_INTEGER now;

    if ( invFreq == 0.0 )
    {
        LARGE_INTEGER freq;

        QueryPerformanceFrequency( &freq );
        invFreq = 1.0 / ( double )freq.QuadPart;
    }

    QueryPerformanceCounter( &now );

    return ( double )now.QuadPart * invFreq;
}

#define GpuSeconds GpuTrace_Seconds

void GpuTrace_Stats( double *rays, double *busySeconds, double *waitSeconds )
{
    *rays        = gpu.statRays;
    *busySeconds = gpu.statBusy;
    *waitSeconds = gpu.statWait;
}

int GpuTrace_TraceJobs( const GpuJob_t *jobs, int jobCount, GpuHit_t *hits )
{
    int    ok = 1;
    int    perSlice;
    double queued = GpuSeconds();
    double started;
    double rays = ( double )jobCount * gpu.directionCount;

    if ( !gpu.directionCount )
        return 0;

    perSlice = GPU_MAX_RAYS_PER_DISPATCH / gpu.directionCount;

    EnterCriticalSection( &gpu.lock );

    started = GpuSeconds();
    gpu.statWait += started - queued;

    while ( jobCount > 0 )
    {
        int slice = jobCount < perSlice ? jobCount : perSlice;

        if ( !TraceSlice( jobs, slice, hits ) )
        {
            ok = 0;
            break;
        }

        jobs     += slice;
        hits     += slice * gpu.directionCount;
        jobCount -= slice;
    }

    gpu.statRays += rays;
    gpu.statBusy += GpuSeconds() - started;

    LeaveCriticalSection( &gpu.lock );

    return ok;
}

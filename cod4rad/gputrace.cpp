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
struct Ray   { float3 start; float p0; float3 end; float p1; };
struct Hit   { int tri; float u; float v; float frac; };

StructuredBuffer<Node>  gNodes   : register( t0 );
StructuredBuffer<Leaf>  gLeafs   : register( t1 );
StructuredBuffer<int>   gTriRefs : register( t2 );
StructuredBuffer<Tri>   gTris    : register( t3 );
StructuredBuffer<Mtl>   gMtls    : register( t4 );
StructuredBuffer<uint>  gMasks   : register( t5 );
StructuredBuffer<Ray>   gRays    : register( t6 );
RWStructuredBuffer<Hit> gHits    : register( u0 );

cbuffer Params : register( b0 ) { uint gRayCount; uint gPad0; uint gPad1; uint gPad2; };

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

    Ray ray = gRays[id.x];
    float3 start = ray.start;
    float3 end = ray.end;
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

    ID3D11Buffer              *rayBuffer;
    ID3D11ShaderResourceView  *rayView;
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

    /* Ray input (CPU write) */
    memset( &desc, 0, sizeof( desc ) );
    desc.ByteWidth           = GPU_MAX_RAYS_PER_DISPATCH * 32;
    desc.Usage               = D3D11_USAGE_DYNAMIC;
    desc.BindFlags           = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = 32;
    hr = gpu.device->CreateBuffer( &desc, NULL, &gpu.rayBuffer );
    if ( SUCCEEDED( hr ) )
        hr = gpu.device->CreateShaderResourceView( gpu.rayBuffer, NULL, &gpu.rayView );

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
    SafeRelease( gpu.rayView );
    SafeRelease( gpu.rayBuffer );

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

static int TraceSlice( const GpuRay_t *rays, GpuHit_t *hits, int count )
{
    D3D11_MAPPED_SUBRESOURCE mapped;
    ID3D11ShaderResourceView *views[7];
    ID3D11ShaderResourceView *noViews[7] = { 0 };
    ID3D11UnorderedAccessView *uav = gpu.hitView;
    ID3D11UnorderedAccessView *noUav = NULL;
    ID3D11Buffer *cb = gpu.params;
    D3D11_BOX box;
    int i;

    /* Rays: 24 bytes of data in a 32 byte record */
    if ( FAILED( gpu.context->Map( gpu.rayBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
        return 0;

    for ( i = 0; i < count; i++ )
    {
        float *dst = ( float * )( ( char * )mapped.pData + i * 32 );

        dst[0] = rays[i].start[0]; dst[1] = rays[i].start[1]; dst[2] = rays[i].start[2];
        dst[3] = 0.0f;
        dst[4] = rays[i].end[0];   dst[5] = rays[i].end[1];   dst[6] = rays[i].end[2];
        dst[7] = 0.0f;
    }
    gpu.context->Unmap( gpu.rayBuffer, 0 );

    if ( SUCCEEDED( gpu.context->Map( gpu.params, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
    {
        unsigned *p = ( unsigned * )mapped.pData;

        p[0] = ( unsigned )count;
        p[1] = p[2] = p[3] = 0;
        gpu.context->Unmap( gpu.params, 0 );
    }

    for ( i = 0; i < 6; i++ )
        views[i] = gpu.sceneViews[i];
    views[6] = gpu.rayView;

    gpu.context->CSSetShader( gpu.shader, NULL, 0 );
    gpu.context->CSSetShaderResources( 0, 7, views );
    gpu.context->CSSetUnorderedAccessViews( 0, 1, &uav, NULL );
    gpu.context->CSSetConstantBuffers( 0, 1, &cb );

    gpu.context->Dispatch( ( UINT )( ( count + GPU_GROUP_SIZE - 1 ) / GPU_GROUP_SIZE ), 1, 1 );

    gpu.context->CSSetShaderResources( 0, 7, noViews );
    gpu.context->CSSetUnorderedAccessViews( 0, 1, &noUav, NULL );

    box.left  = 0; box.right  = ( UINT )count * sizeof( GpuHit_t );
    box.top   = 0; box.bottom = 1;
    box.front = 0; box.back   = 1;
    gpu.context->CopySubresourceRegion( gpu.hitStaging, 0, 0, 0, 0, gpu.hitBuffer, 0, &box );

    /* Map blocks until the dispatch has finished */
    if ( SUCCEEDED( gpu.context->Map( gpu.hitStaging, 0, D3D11_MAP_READ, 0, &mapped ) ) )
    {
        memcpy( hits, mapped.pData, ( size_t )count * sizeof( GpuHit_t ) );
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

int GpuTrace_Trace( const GpuRay_t *rays, GpuHit_t *hits, int count )
{
    int    ok = 1;
    int    total = count;
    double queued = GpuSeconds();
    double started;

    EnterCriticalSection( &gpu.lock );

    started = GpuSeconds();
    gpu.statWait += started - queued;

    while ( count > 0 )
    {
        int slice = count < GPU_MAX_RAYS_PER_DISPATCH ? count : GPU_MAX_RAYS_PER_DISPATCH;

        if ( !TraceSlice( rays, hits, slice ) )
        {
            ok = 0;
            break;
        }

        rays  += slice;
        hits  += slice;
        count -= slice;
    }

    gpu.statRays += total;
    gpu.statBusy += GpuSeconds() - started;

    LeaveCriticalSection( &gpu.lock );

    return ok;
}

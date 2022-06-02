#include "pch.h"
#include "MainWindow.h"

namespace shaders
{
    namespace pixel
    {
        #include "PixelShader.h"
    }
    namespace vertex
    {
        #include "VertexShader.h"
    }
}

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::desktop;
    using namespace robmikh::common::uwp;
}

float CLEARCOLOR[] = { 0.39f, 0.58f, 0.93f, 1.0f }; // RGBA

struct Vertex
{
    winrt::float3 pos;
    winrt::float3 color;
};

template<typename T>
winrt::com_ptr<ID3D11Buffer> CreateBuffer(winrt::com_ptr<ID3D11Device> d3dDevice, std::vector<T> const& data, uint32_t bindFlags);

int __stdcall WinMain(HINSTANCE, HINSTANCE, PSTR, int)
{
    // Initialize COM
    winrt::init_apartment(winrt::apartment_type::single_threaded);

    // Create the DispatcherQueue that the compositor needs to run
    auto controller = util::CreateDispatcherQueueControllerForCurrentThread();

    // Create our window and visual tree
    auto window = MainWindow(L"HelloTriangle11", 800, 600);
    auto compositor = winrt::Compositor();
    auto target = window.CreateWindowTarget(compositor);
    auto root = compositor.CreateSpriteVisual();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
    root.Brush(compositor.CreateColorBrush(winrt::Colors::White()));
    target.Root(root);

    // Init D3D11
    uint32_t deviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    deviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    auto d3dDevice = util::CreateD3DDevice(deviceFlags);
    winrt::com_ptr<ID3D11DeviceContext> d3dContext;
    d3dDevice->GetImmediateContext(d3dContext.put());

    // Create our swapchain
    auto swapChain = util::CreateDXGISwapChain(d3dDevice, 800, 600, DXGI_FORMAT_B8G8R8A8_UNORM, 3);
    auto surface = util::CreateCompositionSurfaceForSwapChain(compositor, swapChain.get());
    auto visual = compositor.CreateSpriteVisual();
    visual.RelativeSizeAdjustment({ 1.0f, 1.0f });
    visual.Brush(compositor.CreateSurfaceBrush(surface));
    root.Children().InsertAtTop(visual);

    // Setup our back buffer
    winrt::com_ptr<ID3D11Texture2D> backBuffer;
    winrt::check_hresult(swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
    winrt::com_ptr<ID3D11RenderTargetView> rtv;
    winrt::check_hresult(d3dDevice->CreateRenderTargetView(backBuffer.get(), nullptr, rtv.put()));
    
    // Load our shaders and input layout
    winrt::com_ptr<ID3D11VertexShader> vertexShader;
    winrt::check_hresult(d3dDevice->CreateVertexShader(
        shaders::vertex::g_main, 
        ARRAYSIZE(shaders::vertex::g_main), 
        nullptr, 
        vertexShader.put()));
    winrt::com_ptr<ID3D11PixelShader> pixelShader;
    winrt::check_hresult(d3dDevice->CreatePixelShader(
        shaders::pixel::g_main,
        ARRAYSIZE(shaders::pixel::g_main),
        nullptr,
        pixelShader.put()));

    std::vector<D3D11_INPUT_ELEMENT_DESC> vertexDesc = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    winrt::com_ptr<ID3D11InputLayout> inputLayout;
    winrt::check_hresult(d3dDevice->CreateInputLayout(
        vertexDesc.data(),
        vertexDesc.size(),
        shaders::vertex::g_main,
        ARRAYSIZE(shaders::vertex::g_main),
        inputLayout.put()));

    // Create our vertex and index buffers
    std::vector<Vertex> vertices =
    {
        { { 0.0f,  0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f } },
        { { 0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f } },
        { {-0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    std::vector<uint16_t> indices =
    {
        0, 1, 2,
    };
    auto vertexBuffer = CreateBuffer<Vertex>(d3dDevice, vertices, D3D11_BIND_VERTEX_BUFFER);
    auto indexBuffer = CreateBuffer<uint16_t>(d3dDevice, indices, D3D11_BIND_INDEX_BUFFER);

    // Setup our pipeline
    std::vector<ID3D11Buffer*> vertexBuffers = { vertexBuffer.get() };
    uint32_t vertexStride = sizeof(Vertex);
    uint32_t offset = 0;
    d3dContext->IASetVertexBuffers(0, vertexBuffers.size(), vertexBuffers.data(), &vertexStride, &offset);
    d3dContext->IASetIndexBuffer(indexBuffer.get(), DXGI_FORMAT_R16_UINT, 0);
    d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    d3dContext->IASetInputLayout(inputLayout.get());
    d3dContext->VSSetShader(vertexShader.get(), nullptr, 0);
    d3dContext->PSSetShader(pixelShader.get(), nullptr, 0);
    std::vector<ID3D11RenderTargetView*> rtvs = { rtv.get() };
    d3dContext->OMSetRenderTargets(rtvs.size(), rtvs.data(), nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;
    viewport.Width = 800.0f;
    viewport.Height = 600.0f;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    d3dContext->RSSetViewports(1, &viewport);  

    // Draw our content
    d3dContext->ClearRenderTargetView(rtv.get(), CLEARCOLOR);
    d3dContext->DrawIndexed(indices.size(), 0, 0);

    // Present!
    winrt::check_hresult(swapChain->Present(0, 0));

    // Message pump
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return util::ShutdownDispatcherQueueControllerAndWait(controller, static_cast<int>(msg.wParam));
}


template<typename T>
winrt::com_ptr<ID3D11Buffer> CreateBuffer(winrt::com_ptr<ID3D11Device> d3dDevice, std::vector<T> const& data, uint32_t bindFlags)
{
    D3D11_SUBRESOURCE_DATA bufferData = {};
    bufferData.pSysMem = reinterpret_cast<const void*>(data.data());
    D3D11_BUFFER_DESC desc = {};
    desc.ByteWidth = sizeof(T) * data.size();
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = bindFlags;
    winrt::com_ptr<ID3D11Buffer> buffer;
    winrt::check_hresult(d3dDevice->CreateBuffer(&desc, &bufferData, buffer.put()));
    return buffer;
}
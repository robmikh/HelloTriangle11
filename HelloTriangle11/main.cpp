#include "pch.h"
#include "MainWindow.h"

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
    auto d3dDevice = util::CreateD3DDevice();
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

    // Clear the back buffer
    d3dContext->ClearRenderTargetView(rtv.get(), CLEARCOLOR);

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

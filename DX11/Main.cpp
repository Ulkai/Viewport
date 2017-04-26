#include "windows.h"
#include <stdio.h>  
#include <wtypes.h>   

#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")

IDXGISwapChain *SwapChain;
ID3D11Device *Device;
ID3D11DeviceContext *DeviceContext;
ID3D11RenderTargetView *RenderTargetView;

int WindowWidth = 640;
int WindowHeight = 480;

void InitD3D11(HWND Window)
{
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = { 0 };
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.Width = WindowWidth;
	SwapChainDesc.BufferDesc.Height = WindowHeight;
	SwapChainDesc.SampleDesc.Count = 1;
	SwapChainDesc.SampleDesc.Quality = 0;
	SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	SwapChainDesc.BufferCount = 1;
	SwapChainDesc.OutputWindow = Window;
	SwapChainDesc.Windowed = TRUE;
	SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	SwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	hr = D3D11CreateDeviceAndSwapChain(NULL
		, D3D_DRIVER_TYPE_HARDWARE
		, NULL, NULL, NULL, NULL
		, D3D11_SDK_VERSION
		, &SwapChainDesc, &SwapChain, &Device, NULL, &DeviceContext);	

	ID3D11Texture2D *BackBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	hr = Device->CreateRenderTargetView(BackBuffer, NULL, &RenderTargetView);
	hr = BackBuffer->Release();
	DeviceContext->OMSetRenderTargets(1, &RenderTargetView, NULL);

	D3D11_VIEWPORT Viewport = { 0 };
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = 640;
	Viewport.Height = 480;

	DeviceContext->RSSetViewports(1, &Viewport);
}

void ShutdownD3D11()
{
	SwapChain->SetFullscreenState(FALSE, NULL);

	SwapChain->Release();
	Device->Release();
	DeviceContext->Release();
	RenderTargetView->Release();
}

void RenderFrame()
{
	float Color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
	DeviceContext->ClearRenderTargetView(RenderTargetView, Color);
	SwapChain->Present(0, 0);
}

void Log(wchar_t* Format, ...)
{
	wchar_t Buffer[256];
	va_list Args;
	va_start(Args, Format);
	_vsnwprintf_s(Buffer, _countof(Buffer), _TRUNCATE, Format, Args);
	va_end(Args);
	::OutputDebugString(Buffer);
}

LRESULT CALLBACK WindowProc(HWND Window, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
		case WM_DESTROY:
			::PostQuitMessage(0);
			return 0;
	}
	return ::DefWindowProc(Window, Msg, wParam, lParam);
}

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
	wchar_t *ClassName = L"ViewportClass";
	wchar_t *WindowName = L"Viewport";
	WNDCLASSEX WndClass = { 0 };
	WndClass.cbSize = sizeof(WndClass);
	WndClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	WndClass.lpszClassName = ClassName;
	WndClass.hInstance = Instance;
	WndClass.lpfnWndProc = WindowProc;

	int ScreenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int ScreenHeight = ::GetSystemMetrics(SM_CYSCREEN);

	ATOM Atom = ::RegisterClassEx(&WndClass);
	HWND Window = ::CreateWindowEx(NULL, ClassName, WindowName, 0
		, (ScreenWidth - WindowWidth) / 2
		, (ScreenHeight - WindowHeight) / 2
		, WindowWidth, WindowHeight
		, NULL, NULL, Instance, NULL);

	::ShowWindow(Window, CmdShow);

	InitD3D11(Window);

	MSG Msg;
	while (true)
	{
		while (::PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&Msg);
			::DispatchMessage(&Msg);
		}
		if (Msg.message == WM_QUIT)
		{
			break;
		}
		RenderFrame();
	}

	ShutdownD3D11();

	return 0;
}

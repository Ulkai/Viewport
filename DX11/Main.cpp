#include "windows.h"
#include <stdio.h>  
#include <wtypes.h>   
#include <D3D11.h>
#include <D3Dcompiler.h>
#include <DDSTextureLoader.h>
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D3dcompiler.lib")

IDXGISwapChain *SwapChain;
ID3D11Device *Device;
ID3D11DeviceContext *Context;
ID3D11RenderTargetView *RenderTargetView;
ID3D11VertexShader *VS;
ID3D11PixelShader *PS;
ID3D11InputLayout *Input;
ID3D11Buffer *VB;

int WindowWidth = 640;
int WindowHeight = 480;

void Log(wchar_t* Format, ...)
{
	wchar_t Buffer[256];
	va_list Args;
	va_start(Args, Format);
	_vsnwprintf_s(Buffer, _countof(Buffer), _TRUNCATE, Format, Args);
	va_end(Args);
	::OutputDebugString(Buffer);
}

struct Vertex
{
	float Position[3];
	float Color[4];
};

void InitD3D11(HWND Window)
{
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC SwapChainDesc = { 0 };
	SwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SwapChainDesc.BufferDesc.Width = WindowWidth;
	SwapChainDesc.BufferDesc.Height = WindowHeight;
	SwapChainDesc.SampleDesc.Count = 2;
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
		, &SwapChainDesc, &SwapChain, &Device, NULL, &Context);

	// Output Merger
	ID3D11Texture2D *BackBuffer;
	hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
	hr = Device->CreateRenderTargetView(BackBuffer, NULL, &RenderTargetView);
	hr = BackBuffer->Release();
	Context->OMSetRenderTargets(1, &RenderTargetView, NULL);

	// Rasterizer Stage
	D3D11_VIEWPORT Viewport = { 0 };
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.Width = 640;
	Viewport.Height = 480;
	Context->RSSetViewports(1, &Viewport);

	// Vertex Shader
	ID3DBlob *VSBlob;
	D3DReadFileToBlob(L"VertexShader.cso", &VSBlob);
	Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &VS);
	Context->VSSetShader(VS, NULL, 0);

	// Pixel Shader
	ID3DBlob *PSBlob;
	D3DReadFileToBlob(L"PixelShader.cso", &PSBlob);
	Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), NULL, &PS);
	Context->PSSetShader(PS, NULL, 0);

	// Input Assembly
	D3D11_INPUT_ELEMENT_DESC InputDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	Device->CreateInputLayout(InputDesc, 2, VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &Input);
	Context->IASetInputLayout(Input);
	Vertex Mesh[] =
	{
		{ 0.0f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f },
		{ 0.45f, -0.5, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f },
		{ -0.45f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f },
	};
	D3D11_BUFFER_DESC VBDesc = { 0 };
	VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	VBDesc.ByteWidth = sizeof(Mesh);
	VBDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	VBDesc.MiscFlags = 0;
	VBDesc.StructureByteStride = 0;
	VBDesc.Usage = D3D11_USAGE_DYNAMIC;
	Device->CreateBuffer(&VBDesc, NULL, &VB);
	D3D11_MAPPED_SUBRESOURCE Mapped;
	Context->Map(VB, 0, D3D11_MAP_WRITE_DISCARD, NULL, &Mapped);
	memcpy(Mapped.pData, Mesh, sizeof(Mesh));
	Context->Unmap(VB, 0);
	UINT Stride = sizeof(Vertex);
	UINT Offset = 0;
	Context->IASetVertexBuffers(0, 1, &VB, &Stride, &Offset);
	Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Texture
}

void ShutdownD3D11()
{
	SwapChain->SetFullscreenState(FALSE, NULL);

	SwapChain->Release();
	Device->Release();
	Context->Release();
	RenderTargetView->Release();
	VS->Release();
	PS->Release();
	Input->Release();
	VB->Release();
}

void RenderFrame()
{
	float Color[4] = { 1.0f, 0.5f, 0.0f, 1.0f };
	Context->ClearRenderTargetView(RenderTargetView, Color);
	Context->Draw(3, 0);
	SwapChain->Present(0, 0);
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

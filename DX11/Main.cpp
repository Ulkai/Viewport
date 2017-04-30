#include "windows.h"
#include <stdio.h>  
#include <wtypes.h>   
#include <D3D11.h>
#include <D3Dcompiler.h>
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "D3dcompiler.lib")

IDXGISwapChain *SwapChain = nullptr;
ID3D11Device *Device = nullptr;
ID3D11DeviceContext *Context = nullptr;
ID3D11RenderTargetView *RenderTargetView = nullptr;
ID3D11VertexShader *VS = nullptr;
ID3D11PixelShader *PS = nullptr;
ID3D11InputLayout *Input = nullptr;
ID3D11Buffer *VB = nullptr;
ID3D11Texture2D *Texture = nullptr;
ID3D11ShaderResourceView *TextureSRV = nullptr;
ID3D11SamplerState *SamplerState = nullptr;

int WindowWidth = 1024;
int WindowHeight = 1024;

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
	float UV[2];
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

	UINT CreationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT
#if defined(_DEBUG)
		| D3D11_CREATE_DEVICE_DEBUG
#endif
		;

	hr = D3D11CreateDeviceAndSwapChain
		( nullptr
		, D3D_DRIVER_TYPE_HARDWARE
		, nullptr
		, CreationFlags
		, nullptr
		, 0
		, D3D11_SDK_VERSION
		, &SwapChainDesc, &SwapChain, &Device, nullptr, &Context);

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
	Viewport.Width = (float)WindowWidth;
	Viewport.Height = (float)WindowHeight;
	Context->RSSetViewports(1, &Viewport);

	// Vertex Shader
	ID3DBlob *VSBlob;
	D3DReadFileToBlob(L"VertexShader.cso", &VSBlob);
	Device->CreateVertexShader(VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), NULL, &VS);
	Context->VSSetShader(VS, NULL, 0);
		
	{// Input Assembly
		D3D11_INPUT_ELEMENT_DESC InputDesc[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};
		Device->CreateInputLayout(InputDesc, ARRAYSIZE(InputDesc), VSBlob->GetBufferPointer(), VSBlob->GetBufferSize(), &Input);
		Context->IASetInputLayout(Input);
		Vertex Mesh[] =
		{
			{ 0.00f,  0.5f, 0.0f, 0.5f, 0.0f },
			{ 0.45f, -0.5f, 0.0f, 0.1f, 1.0f },
			{-0.45f, -0.5f, 0.0f, 0.9f, 1.0f },
		};
		D3D11_BUFFER_DESC VBDesc = { 0 };
		VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		VBDesc.ByteWidth = sizeof(Mesh);
		VBDesc.CPUAccessFlags = 0;
		VBDesc.MiscFlags = 0;
		VBDesc.StructureByteStride = 0;
		VBDesc.Usage = D3D11_USAGE_DEFAULT;
		Device->CreateBuffer(&VBDesc, NULL, &VB);
		Context->UpdateSubresource(VB, 0, nullptr, Mesh, 0, 0);
		UINT Stride = sizeof(Vertex);
		UINT Offset = 0;
		Context->IASetVertexBuffers(0, 1, &VB, &Stride, &Offset);
		Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	{ // Texture
		const UINT TEX_SIZE = 1024;
		const UINT TEX_MIPS = 11;
		D3D11_TEXTURE2D_DESC TexDesc = { 0 };
		TexDesc.ArraySize = 1;
		TexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		TexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		TexDesc.Width = TEX_SIZE;
		TexDesc.Height = TEX_SIZE;
		TexDesc.MipLevels = TEX_MIPS;
		TexDesc.SampleDesc.Count = 1;
		TexDesc.Usage = D3D11_USAGE_DEFAULT;
		Device->CreateTexture2D(&TexDesc, NULL, &Texture);
		UINT32 *TexData = (UINT32*)malloc(sizeof(UINT32) * TEX_SIZE * TEX_SIZE);
		for (UINT32 Mip = 0; Mip < TEX_MIPS; Mip++)
		{
			UINT32 MipSize = TEX_SIZE >> Mip;
			for (UINT32 Row = 0; Row < MipSize; Row++)
			{
				for (UINT32 Col = 0; Col < MipSize; Col++)
				{
					UINT32 Color = ((Col>>(6-Mip)) + (Row>>(6-Mip)))&1 ? 255 : 127;
					TexData[Row*TEX_SIZE + Col] 
						= 0xFF000000
						| 256 * Row / MipSize << 16 
						| 256 * Col / MipSize << 8
						| Color;
				}
			}
			Context->UpdateSubresource(Texture, Mip, nullptr, TexData, sizeof(UINT32)*TEX_SIZE, 0);
		}
		free(TexData);
		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = -1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		Device->CreateShaderResourceView(Texture, &SRVDesc, &TextureSRV);
		Context->PSSetShaderResources(0, 1, &TextureSRV);
	}

	// Pixel Shader
	ID3DBlob *PSBlob;
	D3DReadFileToBlob(L"PixelShader.cso", &PSBlob);
	Device->CreatePixelShader(PSBlob->GetBufferPointer(), PSBlob->GetBufferSize(), NULL, &PS);
	Context->PSSetShader(PS, NULL, 0);
	D3D11_SAMPLER_DESC SamplerDesc;
	SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SamplerDesc.MipLODBias = 0.0f;
	SamplerDesc.MaxAnisotropy = 1;
	SamplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	SamplerDesc.BorderColor[4] = { 0 };
	SamplerDesc.MinLOD = 0;
	SamplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
	Device->CreateSamplerState(&SamplerDesc, &SamplerState);
	Context->PSSetSamplers(0, 1, &SamplerState);
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
	Texture->Release();
	TextureSRV->Release();
	SamplerState->Release();
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

		case WM_SIZE:
			if (SwapChain)
			{
				Context->OMSetRenderTargets(0, 0, 0);

				// Release all outstanding references to the swap chain's buffers.
				RenderTargetView->Release();

				HRESULT hr;
				// Preserve the existing buffer count and format.
				// Automatically choose the width and height to match the client rect for HWNDs.
				hr = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);

				// Perform error handling here!

				// Get buffer and create a render-target-view.
				ID3D11Texture2D* pBuffer;
				hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D),
					(void**)&pBuffer);
				// Perform error handling here!

				hr = Device->CreateRenderTargetView(pBuffer, NULL,
					&RenderTargetView);
				// Perform error handling here!
				pBuffer->Release();

				Context->OMSetRenderTargets(1, &RenderTargetView, NULL);

				// Set up the viewport.
				D3D11_VIEWPORT vp;
				vp.Width = LOWORD(lParam);
				vp.Height = HIWORD(lParam);
				vp.MinDepth = 0.0f;
				vp.MaxDepth = 1.0f;
				vp.TopLeftX = 0;
				vp.TopLeftY = 0;
				Context->RSSetViewports(1, &vp);
			}
			break;
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
	HWND Window = ::CreateWindowEx( NULL
		, ClassName
		, WindowName
		, WS_OVERLAPPEDWINDOW
		, (ScreenWidth - WindowWidth) / 2
		, (ScreenHeight - WindowHeight) / 2
		, WindowWidth
		, WindowHeight
		, NULL
		, NULL
		, Instance
		, NULL);

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

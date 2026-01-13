// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once


#if defined(IMS_USE_DX11)


struct dx11_helper
{
	ID3D11Device* pd3dDevice = nullptr;
	ID3D11DeviceContext* pd3dDeviceContext = nullptr;
	IDXGISwapChain* pSwapChain = nullptr;
	ID3D11RenderTargetView* mainRenderTargetView = nullptr;

	HRESULT CreateRenderTarget()
	{
		mainRenderTargetView = nullptr;
		HRESULT hr;

		ID3D11Texture2D* pBackBuffer = nullptr;
		hr = pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		if (hr != S_OK)return hr;
		hr = pd3dDevice->CreateRenderTargetView(
			pBackBuffer, nullptr, &mainRenderTargetView);
		pBackBuffer->Release();
		return hr;
	}


	void CleanupRenderTarget()
	{
		if (mainRenderTargetView) {
			mainRenderTargetView->Release();
			mainRenderTargetView = nullptr;
		}
	}


	// Helper functions to use DirectX11
	HRESULT CreateDeviceD3D(HWND hWnd)
	{
		// Setup swap chain
		DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 2;
		sd.BufferDesc.Width = 0;
		sd.BufferDesc.Height = 0;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1;
		sd.SampleDesc.Quality = 0;
		sd.Windowed = TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

		UINT createDeviceFlags = 0;
		//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		D3D_FEATURE_LEVEL featureLevel;
		const D3D_FEATURE_LEVEL featureLevelArray[2] = {
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_0,
		};

		auto ret = D3D11CreateDeviceAndSwapChain(
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			createDeviceFlags,
			featureLevelArray,
			2,
			D3D11_SDK_VERSION,
			&sd,
			&pSwapChain,
			&pd3dDevice,
			&featureLevel,
			&pd3dDeviceContext);

		if (ret == S_OK) {
			ret = CreateRenderTarget();
		}
		return ret;
	}

	void CleanupDeviceD3D()
	{
		CleanupRenderTarget();
		if (pSwapChain) { pSwapChain->Release(); pSwapChain = nullptr; }
		if (pd3dDeviceContext) { pd3dDeviceContext->Release(); pd3dDeviceContext = nullptr; }
		if (pd3dDevice) { pd3dDevice->Release(); pd3dDevice = nullptr; }
	}


	void ClearColor(std::array<float, 4> color)
	{
		pd3dDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
		pd3dDeviceContext->ClearRenderTargetView(mainRenderTargetView, color.data());
	}

	void OnResize()
	{
		// Release all outstanding references to the swap chain's buffers before resizing.
		CleanupRenderTarget();
		pSwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
		CreateRenderTarget();
	}


	uintptr_t upload_texture_RGBA(size_t w, size_t h, void* data)
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Width = (UINT)w;
		desc.Height = (UINT)h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;

		ID3D11Texture2D* pTexture = nullptr;
		D3D11_SUBRESOURCE_DATA subResource;
		subResource.pSysMem = data;
		subResource.SysMemPitch = desc.Width * 4;
		subResource.SysMemSlicePitch = 0;
		pd3dDevice->CreateTexture2D(&desc, &subResource, &pTexture);
		assert(pTexture);

		// Create texture view
		ID3D11ShaderResourceView* tex = nullptr;

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		ZeroMemory(&srvDesc, sizeof(srvDesc));
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.MipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		pd3dDevice->CreateShaderResourceView(pTexture, &srvDesc, &tex);

		pTexture->Release();

		return (uintptr_t)tex;
	}
};

#endif

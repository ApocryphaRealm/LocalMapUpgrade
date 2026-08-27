#include "ShaderManager.h"

#include "Diagnostics.h"

#include "utils/Logger.h"

#include <d3dcompiler.h>

namespace RE::VR
{
	class ImageSpaceManager
	{
	public:
		enum ImageSpaceEffectEnum
		{
			ISLocalMap = 101, // BSImagespaceShaderLocalMap
		};
	};
}

namespace LMU
{
	static constexpr const char* pixelShaderSrc =
	{
#include "ISLocalMap.hlsl.h"
	};

	ShaderManager::ShaderManager()
	{
		std::uint32_t isLocalMapIndex = REL::Module::IsVR() ? RE::VR::ImageSpaceManager::ISLocalMap : RE::ImageSpaceManager::ISLocalMap;

		RE::ImageSpaceEffect* localMapShaderEffect = RE::ImageSpaceManager::GetSingleton()->effects[isLocalMapIndex];

		auto localMapShader = skyrim_cast<RE::BSImagespaceShader*>(localMapShaderEffect);

		diagnostics::RecordLocalMapShaderFound(localMapShader != nullptr);

		if (localMapShader)
		{
			localMapPixelShader = *localMapShader->pixelShaders.begin();

			// Squared Black & White
			squaredShaders.blackNWhite.fogOfWar = CompilePixelShader(pixelShaderSrc);
			squaredShaders.blackNWhite.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "NO_FOG_OF_WAR" });

			// Squared Color
			squaredShaders.color.fogOfWar = CompilePixelShader(pixelShaderSrc, { "COLOR" });
			squaredShaders.color.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "COLOR", "NO_FOG_OF_WAR" });

			// Round Black & White
			roundShaders.blackNWhite.fogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND" });
			roundShaders.blackNWhite.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND", "NO_FOG_OF_WAR" });

			// Round Color
			roundShaders.color.fogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND", "COLOR" });
			roundShaders.color.noFogOfWar = CompilePixelShader(pixelShaderSrc, { "ROUND", "COLOR", "NO_FOG_OF_WAR" });

			isFogOfWarEnabled = settings::mapmenu::localMapFogOfWar;

			SetPixelShaderProperties(shape, style);
		}
		else
		{
			logger::critical("Could not find local map shader");
		}
	}

	REX::W32::ID3D11PixelShader* ShaderManager::CompilePixelShader(const char* a_pixelShaderSrc,
																   const std::vector<const char*>& a_defineNames)
	{
		static constexpr std::uint32_t compileFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;

		// Names the variant being built ("squared black-and-white with fog" is just the one with
		// no defines at all), so a recorded failure says WHICH of the eight failed rather than
		// only that one did. Built once per variant at startup - never in a frame.
		std::string variantName;
		for (const char* defineName : a_defineNames)
		{
			if (!variantName.empty())
			{
				variantName += '+';
			}

			variantName += defineName;
		}
		if (variantName.empty())
		{
			variantName = "<no defines>";
		}

		std::vector<D3D_SHADER_MACRO> pixelShaderMacro;
		pixelShaderMacro.reserve(a_defineNames.size() + 1);

		for (const char* defineName : a_defineNames)
		{
			pixelShaderMacro.push_back(D3D_SHADER_MACRO{ .Name{ defineName }, .Definition{ "" } });
		}
		pixelShaderMacro.push_back(D3D_SHADER_MACRO{ nullptr, nullptr });

		REX::W32::ID3DBlob* pixelShaderBlob = nullptr;
		ID3DBlob* errorBlob;
		if (FAILED(D3DCompile(a_pixelShaderSrc, strlen(a_pixelShaderSrc),
			nullptr, pixelShaderMacro.data(), nullptr,
			"main", "ps_5_0", compileFlags, 0,
			reinterpret_cast<ID3DBlob**>(&pixelShaderBlob), &errorBlob)))
		{
			logger::critical("Pixel shader failed to compile.");
			if (errorBlob)
			{
				logger::critical("{}", static_cast<LPCSTR>(errorBlob->GetBufferPointer()));
			}

			diagnostics::RecordPixelShaderVariant(false,
				std::format("{}: HLSL compilation failed (D3DCompile's own error text is in the log)", variantName));

			// D3DCompile leaves pixelShaderBlob null on failure. Reading its buffer below would
			// be a null dereference on top of the compile failure already logged above.
			return nullptr;
		}

		logger::debug("Pixel shader succesfully compiled.");

		RE::BSGraphics::Renderer* renderer = RE::BSGraphics::Renderer::GetSingleton();
		REX::W32::ID3D11Device* device = renderer ? renderer->GetRuntimeData().forwarder : nullptr;

		if (!device)
		{
			diagnostics::RecordPixelShaderVariant(false,
				std::format("{}: the D3D11 device was not available", variantName));

			logger::critical("D3D11 device is not available; cannot create the pixel shader");

			return nullptr;
		}

		REX::W32::ID3D11PixelShader* pixelShaderProgram = nullptr;

		if (FAILED(device->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(),
			nullptr, &pixelShaderProgram)))
		{
			diagnostics::RecordPixelShaderVariant(false,
				std::format("{}: ID3D11Device::CreatePixelShader failed", variantName));

			logger::critical("Failed to create pixel shader.");
		}
		else
		{
			diagnostics::RecordPixelShaderVariant(true, {});

			logger::debug("Pixel shader succesfully created.");
		}

		return pixelShaderProgram;
	}

	void ShaderManager::ToggleFogOfWarLocalMapShader()
	{
		SetFogOfWar(!isFogOfWarEnabled);
	}

	void ShaderManager::SetFogOfWar(bool a_enabled)
	{
		isFogOfWarEnabled = a_enabled;

		SetPixelShaderProperties(shape, style);

		if (RE::ConsoleLog::IsConsoleMode())
		{
			RE::ConsoleLog::GetSingleton()->Print("Fog of war - %s.", isFogOfWarEnabled ? "ENABLED" : "DISABLED");
		}
	}

	void ShaderManager::SetPixelShaderProperties(PixelShaderProperty::Shape a_shape, PixelShaderProperty::Style a_style)
	{
		// Dragon's Eye Minimap calls this every frame, so this stays a counter plus three stores
		// and nothing more - no formatting, no allocation, no game-state reads.
		diagnostics::RecordPixelShaderPropertiesSet(a_shape == PixelShaderProperty::Shape::kRound,
													a_style == PixelShaderProperty::Style::kColor,
													isFogOfWarEnabled);

		PixelShaderGroup& styleShaders = a_shape == PixelShaderProperty::Shape::kRound ? roundShaders : squaredShaders;
		PixelShaderGroup::FogOfWarGroup& shaders = a_style == PixelShaderProperty::Style::kColor ? styleShaders.color : styleShaders.blackNWhite;
		localMapPixelShader->shader = isFogOfWarEnabled ? shaders.fogOfWar : shaders.noFogOfWar;

		// shape/style are per-instance members read back by GetPixelShaderProperties(), which
		// Dragon's Eye Minimap calls every frame to snapshot the current style before it
		// temporarily swaps in its own round shape for the minimap's own draw, then restores
		// whatever GetPixelShaderProperties() told it afterwards. Leaving these two members
		// unset here meant Get() always returned this singleton's construction-time value, so
		// the minimap's restore step kept reverting the local map back to that frozen startup
		// style on every frame - toggling color in the settings menu changed the pixel shader
		// immediately (this line, above), which is why the full local map screen responded to
		// it, but the very next minimap frame silently discarded the change.
		if (singleton)
		{
			singleton->shape = a_shape;
			singleton->style = a_style;
		}
	}

	void ShaderManager::GetPixelShaderProperties(PixelShaderProperty::Shape& a_shape, PixelShaderProperty::Style& a_style)
	{
		// Counting reads is what separates "no consumer is asking" from "a consumer is asking and
		// getting an answer it then discards" - the exact ambiguity behind the frozen-style bug
		// documented in SetPixelShaderProperties above. Same per-frame cost reasoning as there.
		diagnostics::RecordPixelShaderPropertiesRead();

		a_shape = singleton->shape;
		a_style = singleton->style;
	}
}
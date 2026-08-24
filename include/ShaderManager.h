#pragma once

#include "Settings.h"

#include "API.h"

namespace LMU
{
	class ShaderManager
	{
	public:
		struct PixelShaderGroup
		{
			struct FogOfWarGroup
			{
				REX::W32::ID3D11PixelShader* fogOfWar = nullptr;
				REX::W32::ID3D11PixelShader* noFogOfWar = nullptr;
			};

			FogOfWarGroup blackNWhite;
			FogOfWarGroup color;
		};

		static void InitSingleton()
		{
			static ShaderManager instance;
			singleton = &instance;
		}

		static ShaderManager* GetSingleton() { return singleton; }

		void ToggleFogOfWarLocalMapShader();

		// Sets fog-of-war to a specific value rather than flipping it, so a settings menu
		// checkbox can apply its own state directly instead of only being able to toggle
		// whatever the current state happens to be.
		void SetFogOfWar(bool a_enabled);

		static void SetPixelShaderProperties(PixelShaderProperty::Shape a_shape, PixelShaderProperty::Style a_style);
		static void GetPixelShaderProperties(PixelShaderProperty::Shape& a_shape, PixelShaderProperty::Style& a_style);

	private:
		ShaderManager();

		REX::W32::ID3D11PixelShader* CompilePixelShader(const char* a_pixelShaderSrc,
														const std::vector<const char*>& a_defineNames = { });

		static inline ShaderManager* singleton = nullptr;
		
		static inline PixelShaderGroup squaredShaders;
		static inline PixelShaderGroup roundShaders;

		static inline RE::BSGraphics::PixelShader* localMapPixelShader = nullptr;

		static inline bool& isFogOfWarEnabled = *REL::Relocation<bool*>{ REL::VariantID{ 501260, 359696, 0x1E70DFC } }.get();

		// members
		PixelShaderProperty::Shape shape = PixelShaderProperty::Shape::kSquared;
		PixelShaderProperty::Style style = settings::mapmenu::localMapColor ? PixelShaderProperty::Style::kColor : PixelShaderProperty::Style::kBlackNWhite;
	};
}
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

		// Rebuilds the eight local-map pixel shaders with MIN_ALPHA = a_minOpacity (0..1) and swaps them in. Main thread.
		// A no-op when the value is the one already built. Returns false when a variant failed (the old set stays).
		bool SetMinOpacity(float a_minOpacity);
		static float GetBuiltMinOpacity() { return builtMinOpacity; }

		static void SetPixelShaderProperties(PixelShaderProperty::Shape a_shape, PixelShaderProperty::Style a_style);
		static void GetPixelShaderProperties(PixelShaderProperty::Shape& a_shape, PixelShaderProperty::Style& a_style);

	private:
		ShaderManager();

		REX::W32::ID3D11PixelShader* CompilePixelShader(const char* a_pixelShaderSrc,
														const std::vector<const char*>& a_defineNames = { },
														const char* a_minAlpha = "0.0");

		// all eight variants into a_squared / a_round; false if any failed
		bool CompileAll(const char* a_minAlpha, PixelShaderGroup& a_squared, PixelShaderGroup& a_round);
		static void Release(PixelShaderGroup& a_group);
		static inline float builtMinOpacity = 0.0F;

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
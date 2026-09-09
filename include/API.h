#pragma once

namespace LMU
{
	struct PixelShaderProperty
	{
		enum class Shape
		{
			kSquared,
			kRound
		};

		enum class Style
		{
			kBlackNWhite,
			kColor
		};
	};

	namespace API
	{
		struct Message
		{
			enum Type : std::uint32_t
			{
				kPixelShaderPropertiesHook,
				kExtraMarkerProvider
			};
		};

		struct PixelShaderPropertiesHookMessage : Message
		{
			static constexpr inline Type type = Type::kPixelShaderPropertiesHook;

			void (*SetPixelShaderProperties)(PixelShaderProperty::Shape a_shape, PixelShaderProperty::Style a_style) = {};
			void (*GetPixelShaderProperties)(PixelShaderProperty::Shape& a_shape, PixelShaderProperty::Style& a_style) = {};
		};

		// Lets another plugin contribute markers to the local map without this one knowing anything
		// about what it is marking. Dispatched once at kDataLoaded, exactly like the pixel-shader
		// hook above.
		//
		// The point of the design is that Local Map Upgrade stays AGNOSTIC. It never learns what an
		// item is; it only forwards whatever integer the provider gives it into the Scaleform
		// ExtraMarkerData array, where LocalMap.ExtraMapMarker.IconTypes turns it into a linkage
		// name. So a provider that ships its own extended IconTypes array and its own icon clips can
		// add marker kinds this plugin has never heard of, and with NO provider registered nothing
		// is called and nothing changes.
		struct ExtraMarkerProviderMessage : Message
		{
			static constexpr inline Type type = Type::kExtraMarkerProvider;

			// Handed to a provider so it can contribute one marker. a_iconType indexes the
			// SWF's IconTypes array - 0-5 are this plugin's own actor markers, and a provider
			// shipping a longer array owns everything above that.
			using AddMarkerFn = void (*)(void* a_frame, std::uint32_t a_refHandle,
										 const char* a_description, std::uint32_t a_iconType);

			// Called while the local map is building its markers, once per build. a_frame is
			// opaque and must be passed straight back to a_add.
			using ProviderFn = void (*)(void* a_frame, AddMarkerFn a_add);

			// Returns false if a provider is already registered - first one wins, deliberately,
			// so two add-ons cannot silently fight over the same marker array.
			bool (*RegisterExtraMarkerProvider)(ProviderFn a_provider) = {};
		};

		template <typename T>
		concept valid_message = std::is_base_of_v<Message, T>;

		template <typename MessageT>
			requires valid_message<MessageT> inline const MessageT* TranslateAs(SKSE::MessagingInterface::Message* a_msg)
		{
			if constexpr (std::is_same_v<Message, MessageT>) {
				return static_cast<Message*>(a_msg->data);
			}
			else {
				if (a_msg->type == MessageT::type && a_msg->dataLen == sizeof(MessageT)) {
					return static_cast<MessageT*>(a_msg->data);
				}

				return nullptr;
			}
		}
	}
}
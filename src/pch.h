#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#define NOMINMAX
#define NOGDI
#include <windows.h>
#include "detours/detours.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace stl
{
	namespace detail
	{
		template<class F, class... Args>
		auto invoke_as_non_member(F f, Args... args) {
			using result_t = std::invoke_result_t<F, Args...>;
			using func_t = result_t(*)(Args...);
			auto func = reinterpret_cast<func_t&>(f);
			return func(args...);
		}
	}

	template<class F>
	struct HookData
	{
		template<class... Args>
		auto call_orig(Args... a_args) {
			// std::invoke() doesn't work if function is a member function of class derived from several base classes,
			// in that case compiler does some strange math with 'this' pointer and gets wrong result
			// return std::invoke(orig, a_args...);
			return detail::invoke_as_non_member(orig, a_args...);
		}

		F	hook{nullptr};
		F	orig{nullptr};
	};
	
	template<class T>
	std::uintptr_t write_detour_(std::uintptr_t a_src, T a_dst) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(reinterpret_cast<PVOID*>(&a_src), reinterpret_cast<PVOID&>(a_dst));

		if (DetourTransactionCommit() != NO_ERROR) {
			spdlog::error("failed to attach detour");
		}

		return a_src;
	}

	template <class T>
	std::uintptr_t write_call_(std::uintptr_t a_src, T a_dst) {
		auto& trampoline = SKSE::GetTrampoline();
		SKSE::AllocTrampoline(14);
		return trampoline.write_call<5>(a_src, reinterpret_cast<std::uintptr_t&>(a_dst));
	}

	template<class F>
	void write_detour(std::uintptr_t a_src, HookData<F>& a_hook) {
		auto orig = write_detour_(a_src, a_hook.hook);
		a_hook.orig = reinterpret_cast<F&>(orig);
	}

	template<class F>
	void write_thunk(std::uintptr_t a_src, HookData<F>& a_hook) {
		auto orig = write_call_(a_src, a_hook.hook);
		a_hook.orig = reinterpret_cast<F&>(orig);
	}
}
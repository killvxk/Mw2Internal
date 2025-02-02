#include "stdafx.h"
#include "sdk.h"
#include "xor.hpp"
#include "lazyimporter.h"
#include "memory.h"
#include <map>
#include "defs.h"
#include "globals.h"
#include "xorstr.hpp"
#pragma comment(lib, "user32.lib")
#define DEBASE(a) ((size_t)a - (size_t)(unsigned long long)GetModuleHandleA(NULL))

uintptr_t dwProcessBase;
uint64_t backup = 0, Online_Loot__GetItemQuantity = 0, stackFix = 0;
NTSTATUS(*NtContinue)(PCONTEXT threadContext, BOOLEAN raiseAlert) = nullptr;

DWORD64 resolveRelativeAddress(DWORD64 instr, DWORD offset, DWORD instrSize) {
	return instr == 0ui64 ? 0ui64 : (instr + instrSize + *(int*)(instr + offset));
}

bool compareByte(const char* pData, const char* bMask, const char* szMask) {
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return false;
	return (*szMask) == NULL;
}

DWORD64 findPattern(DWORD64 dwAddress, DWORD64 dwLen, const char* bMask, const char* szMask) {
	DWORD length = (DWORD)strlen(szMask);
	for (DWORD i = 0; i < dwLen - length; i++)
		if (compareByte((const char*)(dwAddress + i), bMask, szMask))
			return (DWORD64)(dwAddress + i);
	return 0ui64;
}

LONG WINAPI TopLevelExceptionHandler(PEXCEPTION_POINTERS pExceptionInfo)
{
	if (pExceptionInfo && pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_ACCESS_VIOLATION)
	{
		if (pExceptionInfo->ContextRecord->R11 == 0xDEEDBEEF89898989)
		{
			pExceptionInfo->ContextRecord->R11 = backup;

			if (pExceptionInfo->ContextRecord->Rip > Online_Loot__GetItemQuantity && pExceptionInfo->ContextRecord->Rip < (Online_Loot__GetItemQuantity + 0x1000))
			{
				pExceptionInfo->ContextRecord->Rip = stackFix;
				pExceptionInfo->ContextRecord->Rax = 1;
			}
			NtContinue(pExceptionInfo->ContextRecord, 0);
		}
	}

	return EXCEPTION_CONTINUE_SEARCH;
}


namespace process
{
	HWND hwnd;

	BOOL CALLBACK EnumWindowCallBack(HWND hWnd, LPARAM lParam)
	{
		DWORD dwPid = 0;
		GetWindowThreadProcessId(hWnd, &dwPid);
		if (dwPid == lParam)
		{
			hwnd = hWnd;
			return FALSE;
		}
		return TRUE;
	}

	HWND get_process_window()
	{
		if (hwnd)
			return hwnd;

		EnumWindows(EnumWindowCallBack, GetCurrentProcessId());

		if (hwnd == NULL)
			Exit();

		return hwnd;
	}
}

namespace g_data
{
	uintptr_t base;
	uintptr_t peb;
	HWND hWind;
	uintptr_t unlocker;
	uintptr_t ddl_loadasset;
	uintptr_t ddl_getrootstate;
	uintptr_t ddl_getdllbuffer;
	uintptr_t ddl_movetoname;
	uintptr_t ddl_setint;
	uintptr_t Dvar_FindVarByName;
	uintptr_t Dvar_SetBoolInternal;
	uintptr_t Dvar_SetInt_Internal;
	uintptr_t Dvar_SetFloat_Internal;
	uintptr_t Camo_Offset_Auto_Test;
	

	uintptr_t Clantag_auto;

	uintptr_t a_parse;
	uintptr_t ddl_setstring;
	uintptr_t ddl_movetopath;
	uintptr_t ddlgetInth;
	uintptr_t visible_base;
	QWORD current_visible_offset;
	QWORD cached_visible_base;
	QWORD last_visible_offset;
	bool MemCompare(const BYTE* bData, const BYTE* bMask, const char* szMask) {
		for (; *szMask; ++szMask, ++bData, ++bMask) {
			if (*szMask == 'x' && *bData != *bMask) {
				return false;
			}
		}
		return (*szMask == NULL);
	}
	uintptr_t PatternScanEx(uintptr_t start, uintptr_t size, const char* sig, const char* mask)
	{
		BYTE* data = new BYTE[size];
		SIZE_T bytesRead;

		(iat(memcpy).get()(data, (LPVOID)start, size));
		//ReadProcessMemory(hProcess, (LPVOID)start, data, size, &bytesRead);

		for (uintptr_t i = 0; i < size; i++)
		{
			if (MemCompare((const BYTE*)(data + i), (const BYTE*)sig, mask)) {
				return start + i;
			}
		}
		delete[] data;
		return NULL;
	}

	uintptr_t FindOffset(uintptr_t start, uintptr_t size, const char* sig, const char* mask, uintptr_t base_offset, uintptr_t pre_base_offset, uintptr_t rindex, bool addRip)
	{
		auto address = PatternScanEx(start, size, sig, mask) + rindex;
		if (!address)
			return 0;
		auto ret = pre_base_offset + *reinterpret_cast<int32_t*>(address + base_offset);

		if (addRip)
		{
			ret = ret + address;
			if (ret)
				return (ret - base);
		}

		return ret;
	}
	
	void init()
	{
		base = (uintptr_t)(iat(GetModuleHandleA).get()("cod.exe"));
		
		hWind = process::get_process_window();
		
		peb = __readgsqword(0x60);
	
	}
}

namespace sdk
{
	const DWORD nTickTime = 64;//64 ms
	bool bUpdateTick = false;
	std::map<DWORD, velocityInfo_t> velocityMap;
	void no_recoil()
	{
		if (sdk::in_game())
		{
			unsigned __int64 r12 = sdk::get_client_info();
			r12 += globals::recoil;
			unsigned __int64 rsi = r12 + 0x4;
			DWORD edx = *(uint64_t*)(r12 + 0xC);
			DWORD ecx = (DWORD)r12;
			ecx ^= edx;
			DWORD eax = (DWORD)((unsigned __int64)ecx + 0x2);
			eax *= ecx;
			ecx = (DWORD)rsi;
			ecx ^= edx;
			DWORD udZero = eax;
			//left, right
			eax = (DWORD)((unsigned __int64)ecx + 0x2);
			eax *= ecx;
			DWORD lrZero = eax;
			*(uint64_t*)(r12, udZero, sizeof(udZero));
			
		}
	}
	void enable_uav()
	{

		
		auto uavptr = *(uint64_t*)(g_data::base + globals::uavbase);
		if (uavptr != 0)
		{
			//*(bool*)(uavptr + auav) = g_Vars->mSettings.b_radar;
			*(int*)(uavptr + 0x41F) = 2;
			//*(int*)(uavptr + uav + 0x5) = 6;
		}

	
	}
	uintptr_t _get_player(int i)
	{
		auto cl_info_base = get_client_info_base();

		if (is_bad_ptr(cl_info_base))return 0;
		
		
			auto base_address = *(uintptr_t*)(cl_info_base);
			if (is_bad_ptr(base_address))return 0;

				return sdk::get_client_info_base() + (i * player_info::size);

	}
	bool in_game()
	{
		auto gameMode = *(int*)(g_data::base + game_mode);
		return  gameMode > 1;
	}

	int get_game_mode()
	{
		return *(int*)(g_data::base + game_mode + 0x4);
	}

	int get_max_player_count()
	{
		return *(int*)(g_data::base + game_mode);
	}

	Vector3 _get_pos(uintptr_t address)
	{
		auto local_pos_ptr = *(uintptr_t*)((uintptr_t)address + player_info::position_ptr);

		if (local_pos_ptr)
		{
			return *(Vector3*)(local_pos_ptr + 0x40);
		}
		return Vector3{};
	}

	uint32_t _get_index(uintptr_t address)
	{
		auto cl_info_base = get_client_info_base();

		if (is_bad_ptr(cl_info_base))return 0;

		return ((uintptr_t)address - cl_info_base) / player_info::size;
	
		
	}

	int _team_id(uintptr_t address)    {

		return *(int*)((uintptr_t)address + player_info::team_id);
	}

	int decrypt_visible_flag(int i, QWORD valid_list)
	{
		auto ptr = valid_list + ((i + i * 8) * 8) + 0xA83; //80 BF ? ? ? ? ? 74 20 80 BF ? ? ? ? ? 74 17
		DWORD dw1 = (*(DWORD*)(ptr + 4) ^ (DWORD)ptr);
		DWORD dw2 = ((dw1 + 2) * dw1);
		BYTE dec_visible_flag = *(BYTE*)(ptr) ^ BYTE1(dw2) ^ (BYTE)dw2;

		return (int)dec_visible_flag;
	}


	bool _is_visible(uintptr_t address)
	{
		if (IsValidPtr<uintptr_t>(&g_data::visible_base))
		{
			uint64_t VisibleList = *(uint64_t*)(g_data::visible_base + 0x108);
			if (is_bad_ptr( VisibleList))
				return false;

			uint64_t rdx = VisibleList + (_get_index(address) * 9 + 0x14E) * 8;
			if (is_bad_ptr(rdx))
				return false;

			DWORD VisibleFlags = (rdx + 0x10) ^ (*(DWORD*)(rdx + 0x14));
			if (is_bad_ptr(VisibleFlags))
				return false;

			DWORD v511 = VisibleFlags * (VisibleFlags + 2);
			if (!v511)
				return false;

			BYTE VisibleFlags1 = *(DWORD*)(rdx + 0x10) ^ v511 ^ BYTE1(v511);
			if (VisibleFlags1 == 3) {
				return true;
			}
		}
		return false;
	}
	// zyy

	uint64_t get_client_info()
	{
		auto baseModuleAddr = g_data::base;
		auto Peb = __readgsqword(0x60);
		uint64_t rax = baseModuleAddr, rbx = baseModuleAddr, rcx = baseModuleAddr, rdx = baseModuleAddr, rdi = baseModuleAddr ,rsi = baseModuleAddr, r8 = baseModuleAddr, r9 = baseModuleAddr, r10 = baseModuleAddr, r11 = baseModuleAddr, r12 = baseModuleAddr, r13 = baseModuleAddr, r14 = baseModuleAddr, r15 = baseModuleAddr;
		rbx = *(uintptr_t*)(baseModuleAddr + 0x138AC798);
		if (!rbx)
			return rbx;
		rdx = ~Peb;              //mov rdx, gs:[rax]
		rax = rbx;              //mov rax, rbx
		rax >>= 0x22;           //shr rax, 0x22
		rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
		rbx ^= rax;             //xor rbx, rax
		rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
		rcx ^= *(uintptr_t*)(baseModuleAddr + 0xA8B30E3);             //xor rcx, [0x000000000842C840]
		rax = baseModuleAddr + 0x1343C359;              //lea rax, [0x0000000010FB5AAF]
		rbx += rdx;             //add rbx, rdx
		rcx = ~rcx;             //not rcx
		rbx += rax;             //add rbx, rax
		rax = 0xD63E4A83CB9A620B;               //mov rax, 0xD63E4A83CB9A620B
		rbx *= *(uintptr_t*)(rcx + 0x11);             //imul rbx, [rcx+0x11]
		rbx -= rdx;             //sub rbx, rdx
		rbx *= rax;             //imul rbx, rax
		rax = 0x57242547CAD98C71;               //mov rax, 0x57242547CAD98C71
		rbx -= rax;             //sub rbx, rax
		return rbx;
	}
	uint64_t get_client_info_base()
	{
		auto baseModuleAddr = g_data::base;
		auto Peb = __readgsqword(0x60);
		uint64_t rax = baseModuleAddr, rbx = baseModuleAddr, rcx = baseModuleAddr, rdx = baseModuleAddr, rdi = baseModuleAddr, rsi = baseModuleAddr, r8 = baseModuleAddr, r9 = baseModuleAddr, r10 = baseModuleAddr, r11 = baseModuleAddr, r12 = baseModuleAddr, r13 = baseModuleAddr, r14 = baseModuleAddr, r15 = baseModuleAddr;
		rdx = *(uintptr_t*)(get_client_info() + 0x10e670);
		if (!rdx)
			return rdx;
		r11 = ~Peb;              //mov r11, gs:[rax]
		rax = r11;              //mov rax, r11
		rax = _rotl64(rax, 0x23);               //rol rax, 0x23
		rax &= 0xF;
		switch (rax) {
		case 0:
		{
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9C4EA]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844F5A9]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x7;            //shr rax, 0x07
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0xE;            //shr rax, 0x0E
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x1C;           //shr rax, 0x1C
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x38;           //shr rax, 0x38
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0xA;            //shr rax, 0x0A
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x14;           //shr rax, 0x14
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x28;           //shr rax, 0x28
			rdx ^= rax;             //xor rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rdx += rbx;             //add rdx, rbx
			rax = 0x6A51BC9BC4AA6767;               //mov rax, 0x6A51BC9BC4AA6767
			rdx *= rax;             //imul rdx, rax
			rax = 0x5447EBF1221B83E6;               //mov rax, 0x5447EBF1221B83E6
			rdx -= rax;             //sub rdx, rax
			rdx ^= r11;             //xor rdx, r11
			rdx ^= rbx;             //xor rdx, rbx
			return rdx;
		}
		case 1:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844F175]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9C045]
			rdx -= r11;             //sub rdx, r11
			rax = baseModuleAddr + 0x6B3C0100;              //lea rax, [0x0000000068F5C056]
			rax = ~rax;             //not rax
			rdx ^= rax;             //xor rdx, rax
			rdx ^= r11;             //xor rdx, r11
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rdx ^= rbx;             //xor rdx, rbx
			rax = rdx;              //mov rax, rdx
			rax >>= 0x24;           //shr rax, 0x24
			rdx ^= rax;             //xor rdx, rax
			rax = 0x4A2A83616AD92661;               //mov rax, 0x4A2A83616AD92661
			rdx *= rax;             //imul rdx, rax
			rax = 0xECFC5B4C57C54F28;               //mov rax, 0xECFC5B4C57C54F28
			rdx += rax;             //add rdx, rax
			rax = 0xF0FDCE631F7BA29F;               //mov rax, 0xF0FDCE631F7BA29F
			rdx ^= rax;             //xor rdx, rax
			return rdx;
		}
		case 2:
		{
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9BBCB]
			rcx = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov rcx, [0x000000000844EC75]
			rax = 0xD511FD9CF85D2C07;               //mov rax, 0xD511FD9CF85D2C07
			rdx *= rax;             //imul rdx, rax
			rdx ^= r11;             //xor rdx, r11
			rax = r11;              //mov rax, r11
			uintptr_t RSP_0xFFFFFFFFFFFFFFCF;
			RSP_0xFFFFFFFFFFFFFFCF = baseModuleAddr + 0x2E433015;           //lea rax, [0x000000002BFCEB99] : RBP+0xFFFFFFFFFFFFFFCF
			rax *= RSP_0xFFFFFFFFFFFFFFCF;          //imul rax, [rbp-0x31]
			rdx += rax;             //add rdx, rax
			rdx -= rbx;             //sub rdx, rbx
			rax = rdx;              //mov rax, rdx
			rax >>= 0x12;           //shr rax, 0x12
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x24;           //shr rax, 0x24
			rdx ^= rax;             //xor rdx, rax
			rax = 0x3EDAD65FDC1034FF;               //mov rax, 0x3EDAD65FDC1034FF
			rdx *= rax;             //imul rdx, rax
			rax = 0x2AE3002A8E8BF08B;               //mov rax, 0x2AE3002A8E8BF08B
			rdx -= rax;             //sub rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= rcx;             //xor rax, rcx
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			return rdx;
		}
		case 3:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844E8D9]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9B7A9]
			rax = rbx + 0x1771cb1b;                 //lea rax, [rbx+0x1771CB1B]
			rax += r11;             //add rax, r11
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0xB;            //shr rax, 0x0B
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x16;           //shr rax, 0x16
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x2C;           //shr rax, 0x2C
			rdx ^= rax;             //xor rdx, rax
			rdx -= r11;             //sub rdx, r11
			rax = 0xFD500870540625B;                //mov rax, 0xFD500870540625B
			rdx *= rax;             //imul rdx, rax
			rax = 0x1BC06434489E44B5;               //mov rax, 0x1BC06434489E44B5
			rdx += rax;             //add rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x1F;           //shr rax, 0x1F
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x3E;           //shr rax, 0x3E
			rdx ^= rax;             //xor rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = 0x600D6B3C699E6524;               //mov rax, 0x600D6B3C699E6524
			rdx += rax;             //add rdx, rax
			return rdx;
		}
		case 4:
		{
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9B1D8]
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);               //mov r9, [0x000000000844E290]
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r9;              //xor rax, r9
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = 0x44DF33AE79D34CE7;               //mov rax, 0x44DF33AE79D34CE7
			rdx *= rax;             //imul rdx, rax
			rdx -= r11;             //sub rdx, r11
			rax = r11;              //mov rax, r11
			rax = ~rax;             //not rax
			rdx ^= rax;             //xor rdx, rax
			rax = baseModuleAddr + 0x3B36;          //lea rax, [0xFFFFFFFFFDB9E80F]
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x27;           //shr rax, 0x27
			rdx ^= rax;             //xor rdx, rax
			rdx -= r11;             //sub rdx, r11
			rax = r11;              //mov rax, r11
			uintptr_t RSP_0xFFFFFFFFFFFFFFA7;
			RSP_0xFFFFFFFFFFFFFFA7 = baseModuleAddr + 0x27B9;               //lea rax, [0xFFFFFFFFFDB9D94F] : RBP+0xFFFFFFFFFFFFFFA7
			rax *= RSP_0xFFFFFFFFFFFFFFA7;          //imul rax, [rbp-0x59]
			rdx += rax;             //add rdx, rax
			rax = r11;              //mov rax, r11
			rax -= rbx;             //sub rax, rbx
			rax += 0xFFFFFFFFD6AD7A46;              //add rax, 0xFFFFFFFFD6AD7A46
			rdx += rax;             //add rdx, rax
			return rdx;
		}
		case 5:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844DDD9]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9ACA9]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x17;           //shr rax, 0x17
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x2E;           //shr rax, 0x2E
			rdx ^= rax;             //xor rdx, rax
			rax = rbx + 0xb7ef;             //lea rax, [rbx+0xB7EF]
			rax += r11;             //add rax, r11
			rdx += rax;             //add rdx, rax
			rdx -= r11;             //sub rdx, r11
			uintptr_t RSP_0xFFFFFFFFFFFFFFBF;
			RSP_0xFFFFFFFFFFFFFFBF = 0x20E3F69C982B8265;            //mov rax, 0x20E3F69C982B8265 : RBP+0xFFFFFFFFFFFFFFBF
			rdx ^= RSP_0xFFFFFFFFFFFFFFBF;          //xor rdx, [rbp-0x41]
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = 0xAEE1A029315E3D4F;               //mov rax, 0xAEE1A029315E3D4F
			rdx *= rax;             //imul rdx, rax
			rax = rbx + 0x618b;             //lea rax, [rbx+0x618B]
			rax += r11;             //add rax, r11
			rdx += rax;             //add rdx, rax
			rax = 0x4F576A9DC4CD39EE;               //mov rax, 0x4F576A9DC4CD39EE
			rdx += rax;             //add rdx, rax
			return rdx;
		}
		case 6:
		{
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);               //mov r9, [0x000000000844D958]
			rdx -= r11;             //sub rdx, r11
			rax = rdx;              //mov rax, rdx
			rax >>= 0x12;           //shr rax, 0x12
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x24;           //shr rax, 0x24
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x21;           //shr rax, 0x21
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x1A;           //shr rax, 0x1A
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x34;           //shr rax, 0x34
			rax ^= rdx;             //xor rax, rdx
			rdx = 0x17CF0497F2D22203;               //mov rdx, 0x17CF0497F2D22203
			rax *= rdx;             //imul rax, rdx
			rdx = rax;              //mov rdx, rax
			rdx >>= 0x25;           //shr rdx, 0x25
			rdx ^= rax;             //xor rdx, rax
			rax = 0xBE5CC72B0AEE64FD;               //mov rax, 0xBE5CC72B0AEE64FD
			rdx ^= rax;             //xor rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r9;              //xor rax, r9
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			return rdx;
		}
		case 7:
		{
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9A352]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844D3F4]
			rcx = r11;              //mov rcx, r11
			rax = baseModuleAddr + 0xDEF0;          //lea rax, [0xFFFFFFFFFDBA7F53]
			rcx ^= rax;             //xor rcx, rax
			rax = 0xF875422C3B24C08F;               //mov rax, 0xF875422C3B24C08F
			rax -= rcx;             //sub rax, rcx
			rdx += rax;             //add rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x16;           //shr rax, 0x16
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x2C;           //shr rax, 0x2C
			rdx ^= rax;             //xor rdx, rax
			rdx ^= rbx;             //xor rdx, rbx
			rax = 0x65FDE940447DEE2B;               //mov rax, 0x65FDE940447DEE2B
			rdx *= rax;             //imul rdx, rax
			rax = 0x39A26EAD2B76265B;               //mov rax, 0x39A26EAD2B76265B
			rdx += rax;             //add rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x1B;           //shr rax, 0x1B
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x36;           //shr rax, 0x36
			rdx ^= rax;             //xor rdx, rax
			return rdx;
		}
		case 8:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844CF51]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB99E21]
			rax = 0xFFFFFFFFFFFF597D;               //mov rax, 0xFFFFFFFFFFFF597D
			rax -= r11;             //sub rax, r11
			rax -= rbx;             //sub rax, rbx
			rdx += rax;             //add rdx, rax
			rdx ^= r11;             //xor rdx, r11
			rax = 0xD0F09E7A8C7613B3;               //mov rax, 0xD0F09E7A8C7613B3
			rdx *= rax;             //imul rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x1F;           //shr rax, 0x1F
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x3E;           //shr rax, 0x3E
			rdx ^= rax;             //xor rdx, rax
			rax = 0x256B3436B62B89E5;               //mov rax, 0x256B3436B62B89E5
			rdx -= rax;             //sub rdx, rax
			rdx += rbx;             //add rdx, rbx
			rcx = r11;              //mov rcx, r11
			rax = baseModuleAddr + 0x7B64B958;              //lea rax, [0x00000000791E54A4]
			rax = ~rax;             //not rax
			rcx = ~rcx;             //not rcx
			rcx += rax;             //add rcx, rax
			rdx ^= rcx;             //xor rdx, rcx
			return rdx;
		}
		case 9:
		{
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB999C5]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844CA72]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x4;            //shr rax, 0x04
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x8;            //shr rax, 0x08
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x10;           //shr rax, 0x10
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x20;           //shr rax, 0x20
			rdx ^= rax;             //xor rdx, rax
			rax = 0x54E648D07B6D0B80;               //mov rax, 0x54E648D07B6D0B80
			rdx += rax;             //add rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rdx ^= rbx;             //xor rdx, rbx
			rax = rdx;              //mov rax, rdx
			rax >>= 0x16;           //shr rax, 0x16
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x2C;           //shr rax, 0x2C
			rax ^= r11;             //xor rax, r11
			rdx ^= rax;             //xor rdx, rax
			rdx -= r11;             //sub rdx, r11
			rax = 0xB0386AF6C89E01ED;               //mov rax, 0xB0386AF6C89E01ED
			rdx *= rax;             //imul rdx, rax
			return rdx;
		}
		case 10:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844C59F]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9946F]
			rax = 0x9332D19135BB918F;               //mov rax, 0x9332D19135BB918F
			rdx *= rax;             //imul rdx, rax
			rax = 0xFFFFFFFF8DA4B362;               //mov rax, 0xFFFFFFFF8DA4B362
			rax -= r11;             //sub rax, r11
			rax -= rbx;             //sub rax, rbx
			rdx += rax;             //add rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rdx ^= r11;             //xor rdx, r11
			rax = rdx;              //mov rax, rdx
			rax >>= 0x8;            //shr rax, 0x08
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x10;           //shr rax, 0x10
			rdx ^= rax;             //xor rdx, rax
			rcx = r11;              //mov rcx, r11
			rcx = ~rcx;             //not rcx
			rax = baseModuleAddr + 0xD1ED;          //lea rax, [0xFFFFFFFFFDBA64B6]
			rcx *= rax;             //imul rcx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x20;           //shr rax, 0x20
			rcx ^= rax;             //xor rcx, rax
			rdx ^= rcx;             //xor rdx, rcx
			rcx = r11;              //mov rcx, r11
			rcx = ~rcx;             //not rcx
			rax = baseModuleAddr + 0x868;           //lea rax, [0xFFFFFFFFFDB99BA1]
			rdx += rax;             //add rdx, rax
			rdx += rcx;             //add rdx, rcx
			rax = r11;              //mov rax, r11
			rax = ~rax;             //not rax
			rax -= rbx;             //sub rax, rbx
			rax -= 0x7CCC6306;              //sub rax, 0x7CCC6306
			rdx ^= rax;             //xor rdx, rax
			return rdx;
		}
		case 11:
		{
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);               //mov r9, [0x000000000844C04F]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x12;           //shr rax, 0x12
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x24;           //shr rax, 0x24
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0xA;            //shr rax, 0x0A
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x14;           //shr rax, 0x14
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x28;           //shr rax, 0x28
			rdx ^= rax;             //xor rdx, rax
			rax = 0x8CABBD467C0219D3;               //mov rax, 0x8CABBD467C0219D3
			rdx *= rax;             //imul rdx, rax
			rax = 0xAB98E88DE9C18818;               //mov rax, 0xAB98E88DE9C18818
			rdx ^= rax;             //xor rdx, rax
			rdx -= r11;             //sub rdx, r11
			rax = r11;              //mov rax, r11
			uintptr_t RSP_0xFFFFFFFFFFFFFFEF;
			RSP_0xFFFFFFFFFFFFFFEF = baseModuleAddr + 0x8516;               //lea rax, [0xFFFFFFFFFDBA14D0] : RBP+0xFFFFFFFFFFFFFFEF
			rax *= RSP_0xFFFFFFFFFFFFFFEF;          //imul rax, [rbp-0x11]
			rdx += rax;             //add rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r9;              //xor rax, r9
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = 0x596E42B1953FE5C1;               //mov rax, 0x596E42B1953FE5C1
			rdx += rax;             //add rdx, rax
			return rdx;
		}
		case 12:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844BB0E]
			rdx ^= r11;             //xor rdx, r11
			rax = baseModuleAddr + 0x1BAF;          //lea rax, [0xFFFFFFFFFDB9A219]
			rdx ^= rax;             //xor rdx, rax
			rdx -= r11;             //sub rdx, r11
			uintptr_t RSP_0xFFFFFFFFFFFFFF9F;
			RSP_0xFFFFFFFFFFFFFF9F = baseModuleAddr + 0x259F56F6;           //lea rax, [0x000000002358E0D4] : RBP+0xFFFFFFFFFFFFFF9F
			rdx += RSP_0xFFFFFFFFFFFFFF9F;          //add rdx, [rbp-0x61]
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = 0x294D76F27D0F6D85;               //mov rax, 0x294D76F27D0F6D85
			rdx -= rax;             //sub rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x8;            //shr rax, 0x08
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x10;           //shr rax, 0x10
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x20;           //shr rax, 0x20
			rdx ^= rax;             //xor rdx, rax
			rax = 0x40AE2552A77DAFE6;               //mov rax, 0x40AE2552A77DAFE6
			rdx -= r11;             //sub rdx, r11
			rdx ^= rax;             //xor rdx, rax
			rax = 0x6425FC1CEAFDBD3B;               //mov rax, 0x6425FC1CEAFDBD3B
			rdx *= rax;             //imul rdx, rax
			return rdx;
		}
		case 13:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844B716]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB985E6]
			rdx += rbx;             //add rdx, rbx
			rax = rdx;              //mov rax, rdx
			rax >>= 0x10;           //shr rax, 0x10
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x20;           //shr rax, 0x20
			rdx ^= rax;             //xor rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = r11;              //mov rax, r11
			rax -= rbx;             //sub rax, rbx
			rdx += rax;             //add rdx, rax
			rax = 0x7AAF0F372FD53CD5;               //mov rax, 0x7AAF0F372FD53CD5
			rdx *= rax;             //imul rdx, rax
			rax = 0x4BE188FD7D45B824;               //mov rax, 0x4BE188FD7D45B824
			rdx -= rax;             //sub rdx, rax
			rdx ^= r11;             //xor rdx, r11
			rax = rdx;              //mov rax, rdx
			rax >>= 0x25;           //shr rax, 0x25
			rdx ^= rax;             //xor rdx, rax
			rax = 0x2F94247E3E6CDFF6;               //mov rax, 0x2F94247E3E6CDFF6
			rdx -= rax;             //sub rdx, rax
			return rdx;
		}
		case 14:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);              //mov r10, [0x000000000844B289]
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB9814E]
			rax = 0x59FC5D34C1D95075;               //mov rax, 0x59FC5D34C1D95075
			rdx += rax;             //add rdx, rax
			rax = 0x113F93E895C764EB;               //mov rax, 0x113F93E895C764EB
			rdx *= rax;             //imul rdx, rax
			rax = baseModuleAddr + 0x3BFCF952;              //lea rax, [0x0000000039B677A5]
			rax = ~rax;             //not rax
			rax ^= r11;             //xor rax, r11
			rdx -= rax;             //sub rdx, rax
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r10;             //xor rax, r10
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rdx -= rbx;             //sub rdx, rbx
			rax = rdx;              //mov rax, rdx
			rax >>= 0x23;           //shr rax, 0x23
			rdx ^= rax;             //xor rdx, rax
			uintptr_t RSP_0xFFFFFFFFFFFFFF9F;
			RSP_0xFFFFFFFFFFFFFF9F = 0x3D4F5BB3C70BE95B;            //mov rax, 0x3D4F5BB3C70BE95B : RBP+0xFFFFFFFFFFFFFF9F
			rdx *= RSP_0xFFFFFFFFFFFFFF9F;          //imul rdx, [rbp-0x61]
			rax = r11;              //mov rax, r11
			uintptr_t RSP_0xFFFFFFFFFFFFFFA7;
			RSP_0xFFFFFFFFFFFFFFA7 = baseModuleAddr + 0x9DA7;               //lea rax, [0xFFFFFFFFFDBA1F00] : RBP+0xFFFFFFFFFFFFFFA7
			rax ^= RSP_0xFFFFFFFFFFFFFFA7;          //xor rax, [rbp-0x59]
			rdx -= rax;             //sub rdx, rax
			return rdx;
		}
		case 15:
		{
			rbx = baseModuleAddr;           //lea rbx, [0xFFFFFFFFFDB97CD0]
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B3129);               //mov r9, [0x000000000844AD89]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x13;           //shr rax, 0x13
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x26;           //shr rax, 0x26
			rdx ^= rax;             //xor rdx, rax
			rax = 0x63FD32E967945525;               //mov rax, 0x63FD32E967945525
			rdx -= rax;             //sub rdx, rax
			rdx += r11;             //add rdx, r11
			rax = 0xB85215B9839B7D9;                //mov rax, 0xB85215B9839B7D9
			rdx += rax;             //add rdx, rax
			rdx ^= rbx;             //xor rdx, rbx
			rax = 0;                //and rax, 0xFFFFFFFFC0000000
			rax = _rotl64(rax, 0x10);               //rol rax, 0x10
			rax ^= r9;              //xor rax, r9
			rax = _byteswap_uint64(rax);            //bswap rax
			rdx *= *(uintptr_t*)(rax + 0x15);             //imul rdx, [rax+0x15]
			rax = rdx;              //mov rax, rdx
			rax >>= 0x5;            //shr rax, 0x05
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0xA;            //shr rax, 0x0A
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x14;           //shr rax, 0x14
			rdx ^= rax;             //xor rdx, rax
			rax = rdx;              //mov rax, rdx
			rax >>= 0x28;           //shr rax, 0x28
			rdx ^= rax;             //xor rdx, rax
			rax = 0xE52EBF353AE32CDB;               //mov rax, 0xE52EBF353AE32CDB
			rdx *= rax;             //imul rdx, rax
			return rdx;
		}
		}
	}
	uint64_t get_bone_ptr()
	{
		auto baseModuleAddr = g_data::base;
		auto Peb = __readgsqword(0x60);
		uint64_t rax = baseModuleAddr, rbx = baseModuleAddr, rcx = baseModuleAddr, rdx = baseModuleAddr, rdi = baseModuleAddr, rsi = baseModuleAddr, r8 = baseModuleAddr, r9 = baseModuleAddr, r10 = baseModuleAddr, r11 = baseModuleAddr, r12 = baseModuleAddr, r13 = baseModuleAddr, r14 = baseModuleAddr, r15 = baseModuleAddr;
		rax = *(uintptr_t*)(baseModuleAddr + 0xE2759F8);
		if (!rax)
			return rax;
		rbx = Peb;              //mov rbx, gs:[rcx]
		rcx = rbx;              //mov rcx, rbx
		rcx >>= 0x1C;           //shr rcx, 0x1C
		rcx &= 0xF;
		//	Globals::clientSwitch = rcx;
		switch (rcx) {
		case 0:
		{
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);               //mov r9, [0x0000000008200923]
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r9;              //xor rcx, r9
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rax += rbx;             //add rax, rbx
			rcx = rax;              //mov rcx, rax
			rax >>= 0x13;           //shr rax, 0x13
			rcx ^= rax;             //xor rcx, rax
			rax = rcx;              //mov rax, rcx
			rax >>= 0x26;           //shr rax, 0x26
			rax ^= rcx;             //xor rax, rcx
			rax -= rbx;             //sub rax, rbx
			rcx = 0xD5A6F9222EC0CD8B;               //mov rcx, 0xD5A6F9222EC0CD8B
			rax *= rcx;             //imul rax, rcx
			rcx = 0xBB2862E8C0DD851B;               //mov rcx, 0xBB2862E8C0DD851B
			rax *= rcx;             //imul rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x4;            //shr rcx, 0x04
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x8;            //shr rcx, 0x08
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x10;           //shr rcx, 0x10
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x20;           //shr rcx, 0x20
			rax ^= rcx;             //xor rax, rcx
			return rax;
		}
		case 1:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000082004D4]
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x1E;           //shr rcx, 0x1E
			rax ^= rcx;             //xor rax, rcx
			rdx = baseModuleAddr + 0x6179D5AB;              //lea rdx, [0x000000005F0E963C]
			rdx -= rbx;             //sub rdx, rbx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x3C;           //shr rcx, 0x3C
			rdx ^= rcx;             //xor rdx, rcx
			rax ^= rdx;             //xor rax, rdx
			rax += rbx;             //add rax, rbx
			rcx = 0xC430FCF5AB246D6;                //mov rcx, 0xC430FCF5AB246D6
			rax += rcx;             //add rax, rcx
			rcx = 0x8A220291A10CAF87;               //mov rcx, 0x8A220291A10CAF87
			rax *= rcx;             //imul rax, rcx
			rcx = 0x7FAA38A95F85A6FD;               //mov rcx, 0x7FAA38A95F85A6FD
			rax ^= rcx;             //xor rax, rcx
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD94C235]
			rax -= rcx;             //sub rax, rcx
			return rax;
		}
		case 2:
		{
			r14 = baseModuleAddr + 0x2281;          //lea r14, [0xFFFFFFFFFD94E120]
			r13 = baseModuleAddr + 0x66A0AFC0;              //lea r13, [0x0000000064356E50]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x0000000008200065]
			rdx = r13;              //mov rdx, r13
			rdx = ~rdx;             //not rdx
			rdx *= rbx;             //imul rdx, rbx
			rcx = rbx;              //mov rcx, rbx
			rcx = ~rcx;             //not rcx
			rdx += rcx;             //add rdx, rcx
			rcx = baseModuleAddr + 0xBA28;          //lea rcx, [0xFFFFFFFFFD9575C6]
			rax += rcx;             //add rax, rcx
			rax += rdx;             //add rax, rdx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x13;           //shr rcx, 0x13
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x26;           //shr rcx, 0x26
			rax ^= rcx;             //xor rax, rcx
			rcx = r14;              //mov rcx, r14
			rcx ^= rbx;             //xor rcx, rbx
			rax += rcx;             //add rax, rcx
			rcx = 0x975CC895B7E831F1;               //mov rcx, 0x975CC895B7E831F1
			rax ^= rcx;             //xor rax, rcx
			rcx = 0x3B97C5DC626E056F;               //mov rcx, 0x3B97C5DC626E056F
			rax *= rcx;             //imul rax, rcx
			rcx = 0x5534067E232C6632;               //mov rcx, 0x5534067E232C6632
			rax += rcx;             //add rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			return rax;
		}
		case 3:
		{
			r13 = baseModuleAddr + 0x50F8B6F5;              //lea r13, [0x000000004E8D701D]
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);               //mov r9, [0x00000000081FFA7F]
			rcx = r13;              //mov rcx, r13
			rcx = ~rcx;             //not rcx
			rcx ^= rbx;             //xor rcx, rbx
			rax += rcx;             //add rax, rcx
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD94B475]
			rax += rcx;             //add rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r9;              //xor rcx, r9
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = 0x60F6A79B0C8456B1;               //mov rcx, 0x60F6A79B0C8456B1
			rax += rcx;             //add rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x1B;           //shr rcx, 0x1B
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x36;           //shr rcx, 0x36
			rax ^= rcx;             //xor rax, rcx
			rcx = 0x648FCA6FE7D44377;               //mov rcx, 0x648FCA6FE7D44377
			rax -= rcx;             //sub rax, rcx
			rcx = 0xC462FCF18E2C2995;               //mov rcx, 0xC462FCF18E2C2995
			rax *= rcx;             //imul rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x16;           //shr rcx, 0x16
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x2C;           //shr rcx, 0x2C
			rax ^= rcx;             //xor rax, rcx
			return rax;
		}
		case 4:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FF504]
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rax -= rbx;             //sub rax, rbx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x18;           //shr rcx, 0x18
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x30;           //shr rcx, 0x30
			rax ^= rcx;             //xor rax, rcx
			rcx = 0xB1A93FB4C084CAB9;               //mov rcx, 0xB1A93FB4C084CAB9
			rax *= rcx;             //imul rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x24;           //shr rcx, 0x24
			rcx ^= rax;             //xor rcx, rax
			rax = 0x352F8A796F79706B;               //mov rax, 0x352F8A796F79706B
			rcx ^= rax;             //xor rcx, rax
			rax = baseModuleAddr;           //lea rax, [0xFFFFFFFFFD94AF39]
			rcx -= rax;             //sub rcx, rax
			rax = rbx + 0xffffffffa5917e54;                 //lea rax, [rbx-0x5A6E81AC]
			rax += rcx;             //add rax, rcx
			rcx = 0xA920BAB7A21DDE47;               //mov rcx, 0xA920BAB7A21DDE47
			rax *= rcx;             //imul rax, rcx
			return rax;
		}
		case 5:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FF084]
			rdx = baseModuleAddr + 0x661;           //lea rdx, [0xFFFFFFFFFD94B454]
			uintptr_t RSP_0x78;
			RSP_0x78 = 0x19A86082B9386E61;          //mov rcx, 0x19A86082B9386E61 : RSP+0x78
			rax ^= RSP_0x78;                //xor rax, [rsp+0x78]
			rcx = rbx;              //mov rcx, rbx
			rcx = ~rcx;             //not rcx
			uintptr_t RSP_0x30;
			RSP_0x30 = baseModuleAddr + 0x89B8;             //lea rcx, [0xFFFFFFFFFD9537E2] : RSP+0x30
			rcx ^= RSP_0x30;                //xor rcx, [rsp+0x30]
			rax -= rcx;             //sub rax, rcx
			rcx = 0x6CF5D40C805C3929;               //mov rcx, 0x6CF5D40C805C3929
			rax *= rcx;             //imul rax, rcx
			rax -= rbx;             //sub rax, rbx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x23;           //shr rcx, 0x23
			rax ^= rcx;             //xor rax, rcx
			rcx = 0xEA2BDCA216FA84E;                //mov rcx, 0xEA2BDCA216FA84E
			rax += rcx;             //add rax, rcx
			rcx = rdx;              //mov rcx, rdx
			rcx = ~rcx;             //not rcx
			rcx -= rbx;             //sub rcx, rbx
			rax += rcx;             //add rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			return rax;
		}
		case 6:
		{
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);               //mov r9, [0x00000000081FEB65]
			rcx = 0x143119596E0AB6F4;               //mov rcx, 0x143119596E0AB6F4
			rcx -= rbx;             //sub rcx, rbx
			rax += rcx;             //add rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x14;           //shr rcx, 0x14
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x28;           //shr rcx, 0x28
			rax ^= rcx;             //xor rax, rcx
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD94A791]
			rax -= rcx;             //sub rax, rcx
			rcx = 0xD0FF53657C7A437;                //mov rcx, 0xD0FF53657C7A437
			rax += rcx;             //add rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r9;              //xor rcx, r9
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = 0x1946435536018835;               //mov rcx, 0x1946435536018835
			rax *= rcx;             //imul rax, rcx
			rax -= rbx;             //sub rax, rbx
			return rax;
		}
		case 7:
		{
			r11 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r11, [0x00000000081FE754]
			rdx = baseModuleAddr + 0x32D6EFEE;              //lea rdx, [0x00000000306B94C7]
			r8 = 0;                 //and r8, 0xFFFFFFFFC0000000
			r8 = _rotl64(r8, 0x10);                 //rol r8, 0x10
			r8 ^= r11;              //xor r8, r11
			rcx = rbx;              //mov rcx, rbx
			rcx = ~rcx;             //not rcx
			r8 = ~r8;               //not r8
			rax += rcx;             //add rax, rcx
			rax += rdx;             //add rax, rdx
			rax ^= rbx;             //xor rax, rbx
			rax *= *(uintptr_t*)(r8 + 0x13);              //imul rax, [r8+0x13]
			rcx = 0x6B8832A948DD0921;               //mov rcx, 0x6B8832A948DD0921
			rax += rcx;             //add rax, rcx
			rcx = 0x9D382E284DCFD7C7;               //mov rcx, 0x9D382E284DCFD7C7
			rax *= rcx;             //imul rax, rcx
			rcx = 0x4A2F2EC6D9595386;               //mov rcx, 0x4A2F2EC6D9595386
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x7;            //shr rcx, 0x07
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0xE;            //shr rcx, 0x0E
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x1C;           //shr rcx, 0x1C
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x38;           //shr rcx, 0x38
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x15;           //shr rcx, 0x15
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x2A;           //shr rcx, 0x2A
			rax ^= rcx;             //xor rax, rcx
			return rax;
		}
		case 8:
		{
			r14 = baseModuleAddr + 0x98C7;          //lea r14, [0xFFFFFFFFFD953837]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FE133]
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD949B80]
			rax ^= rcx;             //xor rax, rcx
			rcx = 0xB9B101CE6C8E2F91;               //mov rcx, 0xB9B101CE6C8E2F91
			rax *= rcx;             //imul rax, rcx
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD949E77]
			rax -= rcx;             //sub rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0xC;            //shr rcx, 0x0C
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x18;           //shr rcx, 0x18
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x30;           //shr rcx, 0x30
			rax ^= rcx;             //xor rax, rcx
			rax -= rbx;             //sub rax, rbx
			rcx = 0x4F54898D891371A4;               //mov rcx, 0x4F54898D891371A4
			rax += rcx;             //add rax, rcx
			rdx = 0;                //and rdx, 0xFFFFFFFFC0000000
			rcx = r14;              //mov rcx, r14
			rdx = _rotl64(rdx, 0x10);               //rol rdx, 0x10
			rcx ^= rbx;             //xor rcx, rbx
			rax -= rcx;             //sub rax, rcx
			rdx ^= r10;             //xor rdx, r10
			rdx = ~rdx;             //not rdx
			rax *= *(uintptr_t*)(rdx + 0x13);             //imul rax, [rdx+0x13]
			return rax;
		}
		case 9:
		{
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FDCD7]
			rcx = 0x5C495DB1FF8A0C7D;               //mov rcx, 0x5C495DB1FF8A0C7D
			rax *= rcx;             //imul rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD949A32]
			rax -= rcx;             //sub rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x13;           //shr rcx, 0x13
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x26;           //shr rcx, 0x26
			rax ^= rcx;             //xor rax, rcx
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD9497F1]
			rax += rcx;             //add rax, rcx
			rdx = baseModuleAddr;           //lea rdx, [0xFFFFFFFFFD9497D8]
			rdx += rbx;             //add rdx, rbx
			rcx = 0x931F45DADBA6534A;               //mov rcx, 0x931F45DADBA6534A
			rax += rcx;             //add rax, rcx
			rax += rdx;             //add rax, rdx
			rcx = 0x7254D9C4F5E0407F;               //mov rcx, 0x7254D9C4F5E0407F
			rax *= rcx;             //imul rax, rcx
			return rax;
		}
		case 10:
		{
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);               //mov r9, [0x00000000081FD858]
			rcx = 0x16092956D42CB466;               //mov rcx, 0x16092956D42CB466
			rax += rcx;             //add rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x28;           //shr rcx, 0x28
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x22;           //shr rcx, 0x22
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0xB;            //shr rcx, 0x0B
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x16;           //shr rcx, 0x16
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x2C;           //shr rcx, 0x2C
			rax ^= rcx;             //xor rax, rcx
			rcx = 0xC2E2E61ED49F5991;               //mov rcx, 0xC2E2E61ED49F5991
			rax *= rcx;             //imul rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r9;              //xor rcx, r9
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x4;            //shr rcx, 0x04
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x8;            //shr rcx, 0x08
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x10;           //shr rcx, 0x10
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x20;           //shr rcx, 0x20
			rax ^= rcx;             //xor rax, rcx
			rax -= rbx;             //sub rax, rbx
			return rax;
		}
		case 11:
		{
			r13 = baseModuleAddr + 0x3E6FD0B3;              //lea r13, [0x000000003C0461D1]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FD28C]
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x1B;           //shr rcx, 0x1B
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x36;           //shr rcx, 0x36
			rax ^= rcx;             //xor rax, rcx
			r14 = baseModuleAddr;           //lea r14, [0xFFFFFFFFFD948B61]
			rax += r14;             //add rax, r14
			r14 = baseModuleAddr + 0x27799030;              //lea r14, [0x00000000250E1B7A]
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = 0x6E5FECB626B1C472;               //mov rcx, 0x6E5FECB626B1C472
			rax += rcx;             //add rax, rcx
			rdx = r13;              //mov rdx, r13
			rdx = ~rdx;             //not rdx
			rdx ^= rbx;             //xor rdx, rbx
			rcx = 0xF5121CBF37E46BBB;               //mov rcx, 0xF5121CBF37E46BBB
			rax += rcx;             //add rax, rcx
			rax += rdx;             //add rax, rdx
			rcx = r14;              //mov rcx, r14
			rcx ^= rbx;             //xor rcx, rbx
			rax -= rcx;             //sub rax, rcx
			rcx = 0xD2D6E8735A76DE2D;               //mov rcx, 0xD2D6E8735A76DE2D
			rax *= rcx;             //imul rax, rcx
			return rax;
		}
		case 12:
		{
			rdx = baseModuleAddr + 0xF737;          //lea rdx, [0xFFFFFFFFFD958242]
			r13 = baseModuleAddr + 0x1124573F;              //lea r13, [0x000000000EB8E22C]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FCC6E]
			rax += rbx;             //add rax, rbx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x10;           //shr rcx, 0x10
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x20;           //shr rcx, 0x20
			rax ^= rcx;             //xor rax, rcx
			uintptr_t RSP_0x50;
			RSP_0x50 = 0x636BE495B0FA383E;          //mov rcx, 0x636BE495B0FA383E : RSP+0x50
			rax ^= RSP_0x50;                //xor rax, [rsp+0x50]
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = r13;              //mov rcx, r13
			rcx ^= rbx;             //xor rcx, rbx
			rax ^= rcx;             //xor rax, rcx
			rcx = 0x8812EF99851F0715;               //mov rcx, 0x8812EF99851F0715
			rax *= rcx;             //imul rax, rcx
			rcx = rbx;              //mov rcx, rbx
			rcx = ~rcx;             //not rcx
			rcx += rdx;             //add rcx, rdx
			rax ^= rcx;             //xor rax, rcx
			rcx = baseModuleAddr + 0xB69;           //lea rcx, [0xFFFFFFFFFD9494A3]
			rcx -= rbx;             //sub rcx, rbx
			rax += rcx;             //add rax, rcx
			return rax;
		}
		case 13:
		{
			r13 = baseModuleAddr + 0x5EF7;          //lea r13, [0xFFFFFFFFFD94E4FB]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FC7CB]
			rcx = rax;              //mov rcx, rax
			rcx >>= 0xA;            //shr rcx, 0x0A
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x14;           //shr rcx, 0x14
			rax ^= rcx;             //xor rax, rcx
			rdx = rbx;              //mov rdx, rbx
			rcx = rax;              //mov rcx, rax
			rdx = ~rdx;             //not rdx
			rcx >>= 0x28;           //shr rcx, 0x28
			rdx ^= r13;             //xor rdx, r13
			rax ^= rcx;             //xor rax, rcx
			rax += rdx;             //add rax, rdx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r10;             //xor rcx, r10
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			rcx = 0x736E085CD239F4CB;               //mov rcx, 0x736E085CD239F4CB
			rax *= rcx;             //imul rax, rcx
			rax -= rbx;             //sub rax, rbx
			rcx = baseModuleAddr;           //lea rcx, [0xFFFFFFFFFD948112]
			rax ^= rcx;             //xor rax, rcx
			rcx = 0x7DA51A97E3053243;               //mov rcx, 0x7DA51A97E3053243
			rax *= rcx;             //imul rax, rcx
			rcx = 0x785CF31817D043AC;               //mov rcx, 0x785CF31817D043AC
			rax ^= rcx;             //xor rax, rcx
			return rax;
		}
		case 14:
		{
			r13 = baseModuleAddr + 0x16A6;          //lea r13, [0xFFFFFFFFFD9496D5]
			r14 = baseModuleAddr + 0xF57E;          //lea r14, [0xFFFFFFFFFD95759E]
			r9 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);               //mov r9, [0x00000000081FC1F0]
			rcx = rbx;              //mov rcx, rbx
			rcx = ~rcx;             //not rcx
			rcx *= r14;             //imul rcx, r14
			rax += rcx;             //add rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x1A;           //shr rcx, 0x1A
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x34;           //shr rcx, 0x34
			rax ^= rcx;             //xor rax, rcx
			r11 = 0xBE40C084BA769FA;                //mov r11, 0xBE40C084BA769FA
			rcx = r13;              //mov rcx, r13
			rcx *= rbx;             //imul rcx, rbx
			rcx += r11;             //add rcx, r11
			rax += rcx;             //add rax, rcx
			rcx = 0xB92AAB45027C43E2;               //mov rcx, 0xB92AAB45027C43E2
			rax ^= rcx;             //xor rax, rcx
			rcx = baseModuleAddr + 0x4FED906B;              //lea rcx, [0x000000004D820D91]
			rcx += rbx;             //add rcx, rbx
			rax += rcx;             //add rax, rcx
			rcx = 0xBCCB1C832C79BF0B;               //mov rcx, 0xBCCB1C832C79BF0B
			rax *= rcx;             //imul rax, rcx
			rcx = 0;                //and rcx, 0xFFFFFFFFC0000000
			rcx = _rotl64(rcx, 0x10);               //rol rcx, 0x10
			rcx ^= r9;              //xor rcx, r9
			rcx = ~rcx;             //not rcx
			rax *= *(uintptr_t*)(rcx + 0x13);             //imul rax, [rcx+0x13]
			return rax;
		}
		case 15:
		{
			r14 = baseModuleAddr + 0x31081C40;              //lea r14, [0x000000002E9C973C]
			r10 = *(uintptr_t*)(baseModuleAddr + 0xA8B4221);              //mov r10, [0x00000000081FBCBF]
			rdx = 0;                //and rdx, 0xFFFFFFFFC0000000
			rdx = _rotl64(rdx, 0x10);               //rol rdx, 0x10
			rcx = rax;              //mov rcx, rax
			rdx ^= r10;             //xor rdx, r10
			rcx >>= 0x20;           //shr rcx, 0x20
			rdx = ~rdx;             //not rdx
			rax ^= rcx;             //xor rax, rcx
			rcx = r14;              //mov rcx, r14
			rcx ^= rbx;             //xor rcx, rbx
			rax *= *(uintptr_t*)(rdx + 0x13);             //imul rax, [rdx+0x13]
			rax -= rcx;             //sub rax, rcx
			rcx = 0xFC32828FC4E7EFD1;               //mov rcx, 0xFC32828FC4E7EFD1
			rax *= rcx;             //imul rax, rcx
			rax -= rbx;             //sub rax, rbx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x3;            //shr rcx, 0x03
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x6;            //shr rcx, 0x06
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0xC;            //shr rcx, 0x0C
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x18;           //shr rcx, 0x18
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x30;           //shr rcx, 0x30
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x8;            //shr rcx, 0x08
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x10;           //shr rcx, 0x10
			rax ^= rcx;             //xor rax, rcx
			rcx = rax;              //mov rcx, rax
			rcx >>= 0x20;           //shr rcx, 0x20
			rax ^= rcx;             //xor rax, rcx
			rcx = 0x8D793715ED015397;               //mov rcx, 0x8D793715ED015397
			rax *= rcx;             //imul rax, rcx
			return rax;
		}
		}
	}
	uint16_t get_bone_index(uint32_t bone_index)
	{
		auto baseModuleAddr = g_data::base;
		auto Peb = __readgsqword(0x60);
		uint64_t rax = baseModuleAddr, rbx = baseModuleAddr, rcx = baseModuleAddr, rdx = baseModuleAddr, rdi = baseModuleAddr, rsi = baseModuleAddr, r8 = baseModuleAddr, r9 = baseModuleAddr, r10 = baseModuleAddr, r11 = baseModuleAddr, r12 = baseModuleAddr, r13 = baseModuleAddr, r14 = baseModuleAddr, r15 = baseModuleAddr;
		rdi = bone_index;
		rcx = rdi * 0x13C8;
		rax = 0x1B5C5E9652FDACE7;               //mov rax, 0x1B5C5E9652FDACE7
		rax = _umul128(rax, rcx, (uintptr_t*)&rdx);             //mul rcx
		r11 = baseModuleAddr;           //lea r11, [0xFFFFFFFFFD33E338]
		r10 = 0x19E9C4E0C9861BBD;               //mov r10, 0x19E9C4E0C9861BBD
		rdx >>= 0xA;            //shr rdx, 0x0A
		rax = rdx * 0x256D;             //imul rax, rdx, 0x256D
		rcx -= rax;             //sub rcx, rax
		rax = 0x4F9FF77A70376427;               //mov rax, 0x4F9FF77A70376427
		r8 = rcx * 0x256D;              //imul r8, rcx, 0x256D
		rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
		rax = r8;               //mov rax, r8
		rax -= rdx;             //sub rax, rdx
		rax >>= 0x1;            //shr rax, 0x01
		rax += rdx;             //add rax, rdx
		rax >>= 0xD;            //shr rax, 0x0D
		rax = rax * 0x30D1;             //imul rax, rax, 0x30D1
		r8 -= rax;              //sub r8, rax
		rax = 0x70381C0E070381C1;               //mov rax, 0x70381C0E070381C1
		rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
		rax = 0x624DD2F1A9FBE77;                //mov rax, 0x624DD2F1A9FBE77
		rdx >>= 0x6;            //shr rdx, 0x06
		rcx = rdx * 0x92;               //imul rcx, rdx, 0x92
		rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
		rax = r8;               //mov rax, r8
		rax -= rdx;             //sub rax, rdx
		rax >>= 0x1;            //shr rax, 0x01
		rax += rdx;             //add rax, rdx
		rax >>= 0x6;            //shr rax, 0x06
		rcx += rax;             //add rcx, rax
		rax = rcx * 0xFA;               //imul rax, rcx, 0xFA
		rcx = r8 * 0xFC;                //imul rcx, r8, 0xFC
		rcx -= rax;             //sub rcx, rax
		rax = *(uint16_t*)(rcx + r11 * 1 + 0xA96E930);                //movzx eax, word ptr [rcx+r11*1+0xA96E930]
		r8 = rax * 0x13C8;              //imul r8, rax, 0x13C8
		rax = r10;              //mov rax, r10
		rax = _umul128(rax, r8, (uintptr_t*)&rdx);              //mul r8
		rcx = r8;               //mov rcx, r8
		rax = r10;              //mov rax, r10
		rcx -= rdx;             //sub rcx, rdx
		rcx >>= 0x1;            //shr rcx, 0x01
		rcx += rdx;             //add rcx, rdx
		rcx >>= 0xC;            //shr rcx, 0x0C
		rcx = rcx * 0x1D0F;             //imul rcx, rcx, 0x1D0F
		r8 -= rcx;              //sub r8, rcx
		r9 = r8 * 0x3981;               //imul r9, r8, 0x3981
		rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
		rax = r9;               //mov rax, r9
		rax -= rdx;             //sub rax, rdx
		rax >>= 0x1;            //shr rax, 0x01
		rax += rdx;             //add rax, rdx
		rax >>= 0xC;            //shr rax, 0x0C
		rax = rax * 0x1D0F;             //imul rax, rax, 0x1D0F
		r9 -= rax;              //sub r9, rax
		rax = 0xD79435E50D79435F;               //mov rax, 0xD79435E50D79435F
		rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
		rax = 0xA6810A6810A6811;                //mov rax, 0xA6810A6810A6811
		rdx >>= 0x6;            //shr rdx, 0x06
		rcx = rdx * 0x4C;               //imul rcx, rdx, 0x4C
		rax = _umul128(rax, r9, (uintptr_t*)&rdx);              //mul r9
		rax = r9;               //mov rax, r9
		rax -= rdx;             //sub rax, rdx
		rax >>= 0x1;            //shr rax, 0x01
		rax += rdx;             //add rax, rdx
		rax >>= 0x6;            //shr rax, 0x06
		rcx += rax;             //add rcx, rax
		rax = rcx * 0xF6;               //imul rax, rcx, 0xF6
		rcx = r9 * 0xF8;                //imul rcx, r9, 0xF8
		rcx -= rax;             //sub rcx, rax
		r15 = *(uint16_t*)(rcx + r11 * 1 + 0xA9777D0);                //movsx r15d, word ptr [rcx+r11*1+0xA9777D0]
		return r15;
	}

	player_t get_player(int i)
	{
		uint64_t decryptedPtr = get_client_info();

		if (is_valid_ptr (decryptedPtr))
		{
			uint64_t client_info = get_client_info_base();

			if (is_valid_ptr(client_info))
			{
				return player_t(client_info + (i * player_info::size));
			}
		}
		return player_t(NULL);
	}


	player_t get_local_player()
	{
		uint64_t decryptedPtr = get_client_info();

		if (is_bad_ptr(decryptedPtr))return player_t(NULL);

			auto local_index = *(uintptr_t*)(decryptedPtr + player_info::local_index);
			if (is_bad_ptr(local_index))return player_t(NULL);
			auto index = *(int*)(local_index + player_info::local_index_pos);
			return get_player(index);
		
		
	}



	refdef_t* get_refdef()
	{
		uint32_t crypt_0 = *(uint32_t*)(g_data::base + view_port::refdef_ptr);
		uint32_t crypt_1 = *(uint32_t*)(g_data::base + view_port::refdef_ptr + 0x4);
		uint32_t crypt_2 = *(uint32_t*)(g_data::base + view_port::refdef_ptr + 0x8);
		// lower 32 bits
		uint32_t entry_1 = (uint32_t)(g_data::base + view_port::refdef_ptr);
		uint32_t entry_2 = (uint32_t)(g_data::base + view_port::refdef_ptr + 0x4);
		// decryption
		uint32_t _low = entry_1 ^ crypt_2;
		uint32_t _high = entry_2 ^ crypt_2;
		uint32_t low_bit = crypt_0 ^ _low * (_low + 2);
		uint32_t high_bit = crypt_1 ^ _high * (_high + 2);
		auto ret = (refdef_t*)(((QWORD)high_bit << 32) + low_bit);
		if (is_bad_ptr(ret)) return 0;
		else
			return ret;
	}

	Vector3 get_camera_pos()
	{
		Vector3 pos = Vector3{};

		auto camera_ptr = *(uint64_t*)(g_data::base + view_port::camera_ptr);

		if (is_bad_ptr(camera_ptr))return pos;
		
		
		pos = *(Vector3*)(camera_ptr + view_port::camera_pos);
		if (pos.IsZero())return {};
		else
		return pos;
	}


	
    bool bones_to_screen(Vector3* BonePosArray, Vector2* ScreenPosArray, const long Count)
    {
        for (long i = 0; i < Count; ++i)
        {
            if (!world(BonePosArray[i], &ScreenPosArray[i]))
                return false;
        }
        return true;
    }



	bool get_bone_by_player_index(int i, int bone_id, Vector3* Out_bone_pos)
	{
		uint64_t decrypted_ptr = get_bone_ptr();

		if (is_bad_ptr(decrypted_ptr))return false;
		
			unsigned short index = get_bone_index(i);
			if (index != 0)
			{
				uint64_t bone_ptr = *(uint64_t*)(decrypted_ptr + (index * bones::size) + 0xD8);

				if (is_bad_ptr(bone_ptr))return false;
				
					Vector3 bone_pos = *(Vector3*)(bone_ptr + (bone_id * 0x20) + 0x10);

					if (bone_pos.IsZero())return false;

					uint64_t client_info = get_client_info();

					if (is_bad_ptr(client_info))return false;

					
					
						Vector3 BasePos = *(Vector3*)(client_info + bones::bone_base_pos);

						if (BasePos.IsZero())return false;

						bone_pos.x += BasePos.x;
						bone_pos.y += BasePos.y;
						bone_pos.z += BasePos.z;

						*Out_bone_pos = bone_pos;
						return true;
					
				
			}
		
		return false;
	}
	
	int get_player_health(int i)
	{
		uint64_t bgs = *(uint64_t*)(g_data::base + client::name_array);

		if (bgs)
		{
			name_t* pClientInfo = (name_t*)(bgs + client::name_array_padding  +(i * 0xD0));

			if (pClientInfo)
			{
				return pClientInfo->get_health();
			}
		}
		return 0;
	}

	std::string get_player_name(int entityNum)
	{
		uint64_t bgs = *(uint64_t*)(g_data::base + client::name_array);
		if (is_bad_ptr(bgs)) return "";

		if (bgs)
		{
			name_t* clientInfo_ptr = (name_t*)(bgs + client::name_array_padding + (entityNum * 0xD0));
			if (is_bad_ptr(clientInfo_ptr)) return "";

			int length = strlen(clientInfo_ptr->name);
			for (int j = 0; j < length; ++j)
			{
				char ch = clientInfo_ptr->name[j];
				bool is_english = ch >= 0 && ch <= 127;
				if (!is_english)
					return xorstr_("Player");
			}
			return clientInfo_ptr->name;
		}
		return xorstr_("Player");
	}

	void start_tick()
	{
		static DWORD lastTick = 0;
		DWORD t = GetTickCount();
		bUpdateTick = lastTick < t;

		if (bUpdateTick)
			lastTick = t + nTickTime;
	}

	void update_vel_map(int index, Vector3 vPos)
	{
		if (!bUpdateTick)
			return;

		velocityMap[index].delta = vPos - velocityMap[index].lastPos;
		velocityMap[index].lastPos = vPos;
	}

	void clear_map()
	{
		if (!velocityMap.empty()) { velocityMap.clear(); }
	}

	Vector3 get_speed(int index)
	{
		return velocityMap[index].delta;
	}

	Vector3 get_prediction(int index, Vector3 source, Vector3 destination)
	{
		auto local_velocity = get_speed(local_index());
		auto target_velocity = get_speed(index);

		const auto distance = source.distance_to(destination);
		const auto travel_time = distance / globals::bullet_speed;
		auto pred_destination = destination + (target_velocity - local_velocity) * travel_time;
		/*position.x += travel_time * final_speed.x;
		position.y += travel_time * final_speed.y;
		position.z += 0.5 * globals::bullet_gravity * travel_time * travel_time;
		return position;*/

		pred_destination.z += 0.5f * std::fabsf(globals::bullet_gravity) * travel_time;

		return pred_destination;
	}


	Result MidnightSolver(float a, float b, float c)
	{
		Result res;

		double subsquare = b * b - 4 * a * c;

		if (subsquare < 0)
		{
			res.hasResult = false;
			return res;
		}
		else
		{
			res.hasResult = true,
			res.a = (float)((-b + sqrt(subsquare)) / (2 * a));
			res.b = (float)((-b - sqrt(subsquare)) / (2 * a));
		}
		return res;
	}

	Vector3 prediction_solver(Vector3 local_pos, Vector3 position, int index, float bullet_speed)
	{
		Vector3 aimPosition = Vector3().Zero();
		auto target_speed = get_speed(index);

		local_pos -= position; 

		float a = (target_speed.x * target_speed.x) + (target_speed.y * target_speed.y) + (target_speed.z * target_speed.z) - ((bullet_speed * bullet_speed) * 100);
		float b = (-2 * local_pos.x * target_speed.x) + (-2 * local_pos.y * target_speed.y) + (-2 * local_pos.z * target_speed.z);
		float c = (local_pos.x * local_pos.x) + (local_pos.y * local_pos.y) + (local_pos.z * local_pos.z);

		local_pos += position; 

		Result r = MidnightSolver(a, b, c);

		if (r.a >= 0 && !(r.b >= 0 && r.b < r.a))
		{
			aimPosition = position + target_speed * r.a;
		}
		else if (r.b >= 0)
		{
			aimPosition = position + target_speed * r.b;
		}

		return aimPosition;
	
	}

	uint64_t get_visible_base()
	{

		for (int32_t j{}; j <= 0x1770; ++j)
		{
			
			uint64_t vis_base_ptr = *(uint64_t*)(g_data::base + bones::distribute) + (j * 0x190);
			uint64_t cmp_function = *(uint64_t*)(vis_base_ptr + 0x38);

			if (!cmp_function)
				continue;

			//LOGS_ADDR(cmp_function);

			uint64_t about_visible = g_data::base + bones::visible;

			if (cmp_function == about_visible)
			{
				g_data::current_visible_offset = vis_base_ptr;
				return g_data::current_visible_offset;
			}

		}
		return NULL;
	}
	
	bool is_visible(int entityNum) {

		if (!g_data::visible_base)
			return false;

		uint64_t VisibleList = *(uint64_t*)(g_data::last_visible_offset + 0x80);
		if (!VisibleList)
			return false;
		uint64_t v421 = VisibleList + (entityNum * 9 + 0x152) * 8;
		if (!v421)
			return false;
		DWORD VisibleFlags = (v421 + 0x10) ^ *(DWORD*)(v421 + 0x14);
		if (!VisibleFlags)
			return false;
		DWORD v1630 = VisibleFlags * (VisibleFlags + 2);
		if (!v1630)
			return false;
		BYTE VisibleFlags1 = *(DWORD*)(v421 + 0x10) ^ v1630 ^ BYTE1(v1630);
		if (VisibleFlags1 == 3) {
			return true;
		}
		return false;
	}

	void update_last_visible()
	{
		g_data::last_visible_offset = g_data::current_visible_offset;
	}

	// player class methods
	bool player_t::is_valid() {
		if (is_bad_ptr(address))return 0;

		return *(bool*)((uintptr_t)address + player_info::valid);
	}

	bool player_t::is_dead() {
		if (is_bad_ptr(address))return 0;

		auto dead1 = *(bool*)((uintptr_t)address + player_info::dead_1);
		auto dead2 = *(bool*)((uintptr_t)address + player_info::dead_2);
		return !(!dead1 && !dead2 && is_valid());
	}

	int player_t::team_id() {

		if (is_bad_ptr(address))return 0;
		return *(int*)((uintptr_t)address + player_info::team_id);
	}

	int player_t::get_stance() {
		
		if (is_bad_ptr(address))return 4;
		auto ret = *(int*)((uintptr_t)address + player_info::stance);
	

		return ret;
	}


	float player_t::get_rotation()
	{
		if (is_bad_ptr(address))return 0;
		auto local_pos_ptr = *(uintptr_t*)((uintptr_t)address + player_info::position_ptr);

		if (is_bad_ptr(local_pos_ptr))return 0;

		auto rotation = *(float*)(local_pos_ptr + 0x58);

		if (rotation < 0)
			rotation = 360.0f - (rotation * -1);

		rotation += 90.0f;

		if (rotation >= 360.0f)
			rotation = rotation - 360.0f;

		return rotation;
	}

	Vector3 player_t::get_pos() 
	{
		if (is_bad_ptr(address))return {};
		auto local_pos_ptr = *(uintptr_t*)((uintptr_t)address + player_info::position_ptr);

		if (is_bad_ptr(local_pos_ptr))return{};
		else
			return *(Vector3*)(local_pos_ptr + 0x48);
		return Vector3{}; 


	}

	uint32_t player_t::get_index()
	{
		if (is_bad_ptr(this->address))return 0;

		auto cl_info_base = get_client_info_base();
		if (is_bad_ptr(cl_info_base))return 0;
		
		
	return ((uintptr_t)this->address - cl_info_base) / player_info::size;
		
	
	}

	bool player_t::is_visible()
	{
		if (is_bad_ptr(g_data::visible_base))return false;

		if (is_bad_ptr(this->address))return false;
		
			uint64_t VisibleList =*(uint64_t*)(g_data::visible_base + 0x108);
			if (is_bad_ptr(VisibleList))
				return false;

			uint64_t rdx = VisibleList + (player_t::get_index() * 9 + 0x14E) * 8;
			if (is_bad_ptr(rdx))
				return false;

			DWORD VisibleFlags = (rdx + 0x10) ^ *(DWORD*)(rdx + 0x14);
			if (!VisibleFlags)
				return false;

			DWORD v511 = VisibleFlags * (VisibleFlags + 2);
			if (!v511)
				return false;

			BYTE VisibleFlags1 = *(DWORD*)(rdx + 0x10) ^ v511 ^ BYTE1(v511);
			if (VisibleFlags1 == 3) {
				return true;
			}
		
		return false;
	}
	



}


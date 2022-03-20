#include "hacks.h"
#include "gui.h"
#include "globals.h"
#include "memory.h"
#include "vector.h"


#include <thread>


constexpr Vector3 CalculateAngle(
	const Vector3& localPosition,
	const Vector3& enemyPosition,
	const Vector3& viewAngles) noexcept
{
	return ((enemyPosition - localPosition).ToAngle() - viewAngles);
}

int bone;

void hacks::MovementThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		if (globals::bhop)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

			if (!localPlayer)
				continue;


			const auto onGround = mem.Read<bool>(localPlayer + offsets::m_fFlags);

			if (GetAsyncKeyState(VK_SPACE) && onGround & (1 << 0))
				mem.Write<BYTE>(globals::clientAddress + offsets::dwForceJump, 6);
		}

			
	
	}
}



void hacks::VisualsThread(const Memory& mem) noexcept
{


	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));


		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);

		if (!localPlayer)
			continue;


		const auto glowManager = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwGlowObjectManager);

		if (!glowManager)
			continue;

	

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::m_iTeamNum);

	

		for (auto i = 1; i <= 32; i++)
		{
			const auto player = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

			if (!player)
				continue;


			const auto team = mem.Read<std::int32_t>(player + offsets::m_iTeamNum);

			if (team == localTeam)
				continue;

			const auto lifeState = mem.Read<std::int32_t > (player + offsets::m_lifeState);


			if (lifeState != 0)
				continue;

			if (globals::glow)
			{
				const auto glowIndex = mem.Read<std::int32_t>(player + offsets::m_iGlowIndex);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x8, globals::glowColor[0]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0xC, globals::glowColor[1]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x10, globals::glowColor[2]);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x14, globals::glowColor[3]);

				mem.Write(glowManager + (glowIndex * 0x38) + 0x28, true);
				mem.Write(glowManager + (glowIndex * 0x38) + 0x29, false);

			}
			
			if (globals::radar)
				mem.Write(player + offsets::m_bSpotted, true);

			float ra = globals::chamsColor[0];
			float ga = globals::chamsColor[1];
			float ba = globals::chamsColor[2];


			struct Color
			{
				std::uint8_t r{ }, g{ }, b{ };

			};

			constexpr const auto teamColor = Color{ 0, 0, 255 };
			constexpr const auto enemyColor = Color{ 150, 162, 255 };

			if (globals::chams)
			{

				for (auto i = 1; i <= 32; i++)
				{

					if(mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == localTeam)
					{
						mem.Write<Color>(player + offsets::m_clrRender, teamColor);
					}
					else
					{
						mem.Write<Color>(player + offsets::m_clrRender, enemyColor);
					}

					float brightness = 20.f;
					const auto _this = static_cast<std::uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
					mem.Write<std::int32_t>(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<std::uintptr_t*>(&brightness) ^ _this);


				}

			}

			if (globals::aimbot)
			{
				if (!GetAsyncKeyState(VK_XBUTTON1))
					continue;

				const auto localEyePosition = mem.Read<Vector3>(localPlayer + offsets::m_vecOrigin) +
					mem.Read<Vector3>(localPlayer + offsets::m_vecViewOffset);

				const auto& clientState = mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState);


				const auto& localPlayerId =
					mem.Read<std::int32_t>(clientState + offsets::dwClientState_GetLocalPlayer);

				const auto& viewAngles = mem.Read<Vector3>(clientState + offsets::dwClientState_ViewAngles);
				const auto& aimPunch = mem.Read<Vector3>(localPlayer + offsets::m_aimPunchAngle) * 2;


				auto bestFov = 5.f;
				auto bestAngle = Vector3{ };

				for (auto i = 1; i <= 32; ++i)
				{
					if (mem.Read<std::int32_t>(player + offsets::m_iTeamNum) == localTeam)
						continue;

					if (mem.Read<bool>(player + offsets::m_bDormant))
						continue;

					if (!mem.Read<std::int32_t>(player + offsets::m_iHealth))
						continue;

					if (mem.Read<std::int32_t>(player + offsets::m_bSpottedByMask) & (1 << localPlayerId))
					{
						const auto boneMatrix = mem.Read<std::uintptr_t>(player + offsets::m_dwBoneMatrix);

						
						// 8 = head bone 

						

						if (globals::selection == 0)
						{
							bone = 8;
						}
						else if (globals::selection == 1)
						{
							bone = 6;
						}
						else if (globals:: selection == 2) {

							bone = 4;
						}

						const auto playerHeadPosition = Vector3{
							mem.Read<float>(boneMatrix + 0x30 * bone + 0x0C),
							mem.Read<float>(boneMatrix + 0x30 * bone + 0x1C),
							mem.Read<float>(boneMatrix + 0x30 * bone + 0x2C)
						};

						const auto angle = CalculateAngle(
							localEyePosition,
							playerHeadPosition,
							viewAngles + aimPunch
						);

						const auto fov = std::hypot(angle.x, angle.y);

						if (fov < bestFov)
						{
							bestFov = fov;
							bestAngle = angle;
						}
					}
				}

				if (!bestAngle.IsZero())
					mem.Write<Vector3>(clientState + offsets::dwClientState_ViewAngles, viewAngles + bestAngle / 14.f);

			}

			//





		}


	}
}
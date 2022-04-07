#include <vitasdk.h>
#include <vitaGL.h>
#include <imgui_vita.h>
#include <stdio.h>
#include <string>

#define VERSION "0.1"
#define FUNC_TO_NAME(x) #x
#define stringify(x) FUNC_TO_NAME(x)
#define MIN(x, y) (x) < (y) ? (x) : (y)

int _newlib_heap_size_user = 256 * 1024 * 1024;

struct GameSelection {
	char name[128];
	float size;
	bool bilinear;
	bool gles1;
	bool skip_splash;
	bool single_thread;
	bool fake_win_mode;
	bool debug_mode;
	bool debug_shaders;
	GameSelection *next;
};

extern "C" {
void *__wrap_memcpy(void *dest, const void *src, size_t n) {
	return sceClibMemcpy(dest, src, n);
}

void *__wrap_memmove(void *dest, const void *src, size_t n) {
	return sceClibMemmove(dest, src, n);
}

void *__wrap_memset(void *s, int c, size_t n) {
	return sceClibMemset(s, c, n);
}
}

#define LAUNCH_FILE_PATH "ux0:data/gms/launch.txt"

char *desc = nullptr;
char *launch_item = nullptr;

GameSelection *games = nullptr;

char ver_str[64];
char settings_str[512];
float ver_len = 0.0f;
bool calculate_ver_len = true;
bool is_config_invoked = false;
uint32_t oldpad;

int main(int argc, char *argv[]) {
	GameSelection *hovered = nullptr;
	vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);
	ImGui::CreateContext();
	ImGui_ImplVitaGL_Init();
	ImGui_ImplVitaGL_TouchUsage(false);
	ImGui_ImplVitaGL_GamepadUsage(true);
	ImGui::StyleColorsDark();
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

	ImGui::GetIO().MouseDrawCursor = false;
	
	SceUID fd = sceIoDopen("ux0:data/gms");
	SceIoDirent g_dir;
	while (sceIoDread(fd, &g_dir) > 0) {
		char apk_name[256];
		SceIoStat stat;
		sprintf(apk_name, "ux0:data/gms/%s/game.apk", g_dir.d_name);
		if (!sceIoGetstat(apk_name, &stat)) {
			GameSelection *g = (GameSelection *)malloc(sizeof(GameSelection));
			strcpy(g->name, g_dir.d_name);
			g->size = (float)stat.st_size / (1024.0f * 1024.0f);
			g->next = games;
			games = g;
		}
	}
	sceIoDclose(fd);
	
	while (!launch_item) {
		desc = nullptr;
		ImGui_ImplVitaGL_NewFrame();
		
		if (ImGui::BeginMainMenuBar()) {
			ImGui::Text("YoYo Loader Vita");
			if (calculate_ver_len) {
				calculate_ver_len = false;
				sprintf(ver_str, "v.%s (%s)", VERSION, stringify(GIT_VERSION));
				ImVec2 ver_sizes = ImGui::CalcTextSize(ver_str);
				ver_len = ver_sizes.x;
			}
			ImGui::SetCursorPosX(950 - ver_len);
			ImGui::Text(ver_str); 
			ImGui::EndMainMenuBar();
		}
		
		ImGui::SetNextWindowPos(ImVec2(0, 19), ImGuiSetCond_Always);
		ImGui::SetNextWindowSize(ImVec2(560, 524), ImGuiSetCond_Always);
		ImGui::Begin("##main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
		
		GameSelection *g = games;
		ImVec2 config_pos;
		while (g) {
			if (ImGui::Button(g->name, ImVec2(-1.0f, 0.0f))) {
				launch_item = g->name;
			}
			if (ImGui::IsItemHovered()) {
				hovered = g;
			}
			g = g->next;
		}
		ImGui::End();
		
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if (pad.buttons & SCE_CTRL_TRIANGLE && !(oldpad & SCE_CTRL_TRIANGLE) && hovered) {
			is_config_invoked = !is_config_invoked;
			sprintf(settings_str, "%s - Settings", hovered->name);
		}
		oldpad = pad.buttons;
		
		if (is_config_invoked) {
			ImGui::SetNextWindowPos(ImVec2(50, 30), ImGuiSetCond_Always);
			ImGui::SetNextWindowSize(ImVec2(860, 400), ImGuiSetCond_Always);
			ImGui::Begin(settings_str, nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
			ImGui::Checkbox("Force GLES1 Mode", &hovered->gles1);
			ImGui::Checkbox("Force Runner on Main Thread", &hovered->single_thread);
			ImGui::Checkbox("Fake Windows as Platform", &hovered->fake_win_mode);
			ImGui::Separator();
			ImGui::Checkbox("Force Bilinear Filtering", &hovered->bilinear);
			ImGui::Separator();
			ImGui::Checkbox("Skip Splashscreen at Boot", &hovered->skip_splash);
			ImGui::Separator();
			ImGui::Checkbox("Run with Debug Mode", &hovered->debug_mode);
			ImGui::Checkbox("Run with Shaders Debug Mode", &hovered->debug_shaders);
			ImGui::End();
		}
		
		ImGui::SetNextWindowPos(ImVec2(560, 19), ImGuiSetCond_Always);
		ImGui::SetNextWindowSize(ImVec2(400, 524), ImGuiSetCond_Always);
		ImGui::Begin("Info Window", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus);
		if (hovered) {
			ImGui::Text("APK Size: %.2f MBs", hovered->size);
			ImGui::Separator();
			ImGui::Text("Force GLES1 Mode: %s", hovered->gles1 ? "Yes" : "No");
			ImGui::Text("Force Runner on Main Thread: %s", hovered->single_thread ? "Yes" : "No");
			ImGui::Text("Fake Windows as Platform: %s", hovered->fake_win_mode ? "Yes" : "No");
			ImGui::Separator();
			ImGui::Text("Force Bilinear Filtering: %s", hovered->fake_win_mode ? "Yes" : "No");
			ImGui::Separator();
			ImGui::Text("Skip Splashscreen at Boot: %s", hovered->skip_splash ? "Yes" : "No");
			ImGui::Separator();
			ImGui::Text("Run with Debug Mode: %s", hovered->debug_mode ? "Yes" : "No");
			ImGui::Text("Run with Shaders Debug Mode: %s", hovered->debug_shaders ? "Yes" : "No");
			ImGui::Text("");
			ImGui::TextColored(ImVec4(1.0f,1.0f,0.0f,1.0f), "Press Triangle to change settings");
		}
		ImGui::End();
		
		glViewport(0, 0, static_cast<int>(ImGui::GetIO().DisplaySize.x), static_cast<int>(ImGui::GetIO().DisplaySize.y));
		ImGui::Render();
		ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
		vglSwapBuffers(GL_FALSE);
	}

	FILE *f = fopen(LAUNCH_FILE_PATH, "w+");
	fwrite(launch_item, 1, strlen(launch_item), f);
	fclose(f);
	
	char config_path[256];
	sprintf(config_path, "ux0:data/gms/%s/yyl.cfg", launch_item);
	f = fopen(config_path, "w+");
	fprintf(f, "%s=%d\n", "forceGLES1", (int)hovered->gles1);
	fprintf(f, "%s=%d\n", "noSplash", (int)hovered->skip_splash);
	fprintf(f, "%s=%d\n", "forceBilinear", (int)hovered->bilinear);
	fprintf(f, "%s=%d\n", "winMode", (int)hovered->fake_win_mode);
	fprintf(f, "%s=%d\n", "debugMode", (int)hovered->debug_mode);
	fprintf(f, "%s=%d\n", "debugShaders", (int)hovered->debug_shaders);
	fprintf(f, "%s=%d\n", "singleThreaded", (int)hovered->single_thread);
	fclose(f);

	sceAppMgrLoadExec("app0:/loader.bin", NULL, NULL);
	return 0;
}
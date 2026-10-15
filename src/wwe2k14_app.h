// wwe2k14 - ReXGlue Recompiled Project
//
// Minimal first-boot app for WWE 2K14 (TU 2.0.1.0, title 545408B2), adapted from
// WWE '13 Recomp's wwe13_app.h. Everything that patched WWE '13 guest code by
// address (scene resolution/AA, EDRAM tiles, shader-cache seed, cloth handshake,
// present interval, text entry, XMP music) is left out until its 2K14
// equivalents are found. Kept: game-file check, release diagnostics/crash
// report, single-instance lock, freeze logger, keep-awake, F11 fullscreen.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string_view>

#ifndef _WIN32
#include <gtk/gtk.h>
#endif

#include <rex/cvar.h>
#include <rex/diagnostics/freeze_logger.h>
#include <rex/logging.h>
#include <rex/rex_app.h>
#include <rex/ui/keybinds.h>

#include "game_version_check.h"
#include "release_diagnostics.h"
#include "single_instance.h"

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall SetConsoleTitleW(const wchar_t* title);
#endif

class Wwe2k14App : public rex::ReXApp {
 public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(rex::ui::WindowedAppContext& ctx) {
    return std::unique_ptr<Wwe2k14App>(new Wwe2k14App(ctx, "wwe2k14", PPCImageConfig));
  }

  std::string_view GetWindowTitle() const override { return "WWE 2K14"; }

  void OnConfigurePaths(rex::PathConfig& paths) override {
    game_data_root_ = paths.game_data_root;
    user_data_root_ = paths.user_data_root;
    game_file_check_result_ = wwe13::CheckSupportedGameFiles(paths.game_data_root);
  }

  void OnPreSetup(rex::RuntimeConfig& config) override {
    (void)config;
    if (!wwe13::AcquireUserDataInstanceLock(user_data_root_)) {
      REXLOG_ERROR("[wwe2k14] another game instance is already using {}; exiting",
                   rex::path_to_utf8(user_data_root_));
      rex::FlushLogging();
#ifdef _WIN32
      MessageBoxW(nullptr, L"WWE 2K14 is already running. Close the other game window first.",
                  L"WWE 2K14", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
#else
      std::fputs("WWE 2K14 is already running with this game folder.\n", stderr);
#endif
      std::exit(EXIT_FAILURE);
    }
    wwe13::InitializeReleaseDiagnostics(game_data_root_, game_file_check_result_);
    if (game_file_check_result_ == wwe13::GameFileCheckResult::kSupported) {
      REXLOG_INFO("[wwe2k14] validated title ID 545408B2 and title update 2.0.1.0");
    }
#ifdef _WIN32
    SetConsoleTitleW(L"WWE 2K14");
#endif
    // Generic SDK graphics settings the WWE '13 build defaults on (same Yuke's engine).
    rex::cvar::SetFlagByName("depth_float24_convert_in_pixel_shader",
                             EnvIsZero("WWE2K14_DEPTH_F24_CONVERT") ? "false" : "true");
    rex::cvar::SetFlagByName("vsync_host_display",
                             EnvIsZero("WWE2K14_HOST_VSYNC") ? "false" : "true");
    // Music: report the XMP playback controller as (0, 0) like Xenia; the WWE '13 value (title client)
    // makes 2K14 believe the system music player owns playback and mute its menu/entrance music.
    if (std::getenv("REX_XMP_REPORT_TITLE_CONTROLLER") == nullptr) {
      rex::cvar::SetFlagByName("xmp_report_title_controller", "false");
    }
    // 4K: render at 3x the game's 1280x720 (= 3840x2160). Override with REX_RESOLUTION_SCALE=1..8
    // (or WWE2K14_RES_SCALE) for lower-end GPUs.
    if (std::getenv("REX_RESOLUTION_SCALE") == nullptr) {
      const char* scale = std::getenv("WWE2K14_RES_SCALE");
      rex::cvar::SetFlagByName("resolution_scale", scale != nullptr ? scale : "3");
    }
  }

  void OnPostSetup() override {
    rex::diagnostics::freeze_logger::Start();
#ifdef _WIN32
    if (SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED) != 0) {
      REXLOG_INFO("[wwe2k14] keep-awake on");
    }
#endif
    rex::ui::RegisterBind("bind_toggle_fullscreen", "F11", "Toggle fullscreen / windowed", [this] {
      if (auto* w = window()) {
        w->SetFullscreen(!w->IsFullscreen());
      }
    });
  }

  void OnPreLaunchModule() override {
    if (game_file_check_result_ == wwe13::GameFileCheckResult::kSupported) {
      return;
    }
    constexpr char kMessage[] =
        "The selected files are not a supported WWE 2K14 title update 2.0.1.0 set.\n\n"
        "Use your own matching default.xex and default.xexp files from the same game folder.";
#ifdef _WIN32
    const HWND owner = window() ? static_cast<HWND>(window()->GetNativeWindowHandle()) : nullptr;
    MessageBoxA(owner, kMessage, "WWE 2K14", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#else
    wwe13::ShowUnsupportedGameFilesMessage(game_file_check_result_);
    std::fputs(kMessage, stderr);
#endif
    std::exit(EXIT_FAILURE);
  }

  void OnGraphicsSetupFailed(rex::X_STATUS status) override {
    (void)status;
    REXLOG_ERROR("[wwe2k14] no usable graphics card at startup");
    rex::FlushLogging();
#ifdef _WIN32
    MessageBoxW(nullptr,
                L"WWE 2K14 couldn't find a graphics card it can use. Update your graphics driver, "
                L"then try again.",
                L"WWE 2K14", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
#endif
  }

  void OnShutdown() override {
#ifdef _WIN32
    SetThreadExecutionState(ES_CONTINUOUS);
#endif
    rex::ui::UnregisterBind("bind_toggle_fullscreen");
    wwe13::FlushReleaseLogs();
  }

 private:
  static bool EnvIsZero(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr && std::strcmp(value, "0") == 0;
  }

  std::filesystem::path game_data_root_;
  std::filesystem::path user_data_root_;
  wwe13::GameFileCheckResult game_file_check_result_ = wwe13::GameFileCheckResult::kSupported;
};

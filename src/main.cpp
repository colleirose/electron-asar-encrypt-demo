#include "base64.h"
#include "napi.h"
#include "script.h"
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <vector>

#include "aes/aes.hpp"

#include "xor.h"
#include "xorstr.hpp"

#include <cinttypes>
#include <intrin.h>
#include <stdio.h>

#define KEY_LENGTH 32

#define FN_MODULE_PROTOTYPE__COMPILE 0

const std::string debuggerDetected = xorstr_("A debugger has been found running in your system. Please, unload it from memory and restart your program.");
const char integirtyCheckFailed[] = "The program files appear to be corrupted. Please try running an antivirus scan and reinstalling the application.";

namespace {

struct AddonData {
  // std::unordered_map<int, Napi::ObjectReference> modules;
  std::unordered_map<int, Napi::FunctionReference> functions;
};


/* void ConsoleLog(const Napi::Env& env, Napi::Value value) {
  Napi::Object console = env.Global().As<Napi::Object>()
    .Get("console").As<Napi::Object>();
  Napi::Function log = console.Get("log").As<Napi::Function>();
  log.Call(console, { value });
} */

void ConsoleError(const Napi::Env &env, Napi::Value value) {
  Napi::Object console = env.Global()
                             .As<Napi::Object>()
                             .Get(xorstr_("console"))
                             .As<Napi::Object>();
  Napi::Function error = console.Get(xorstr_("error")).As<Napi::Function>();
  error.Call(console, {value});
}

const uint8_t *GetKey() {
  static const uint8_t key[KEY_LENGTH] = {
#include "key.txt"
  };

  return key;
}

Napi::Array GetKeyArray(const Napi::Env &env) {
  const uint8_t *key = GetKey();
  Napi::Array arrkey = Napi::Array::New(env, KEY_LENGTH);
  for (uint32_t i = 0; i < KEY_LENGTH; i++) {
    arrkey.Set(i, key[i]);
  }

  return arrkey;
}

int Pkcs7cut(uint8_t *p, int plen) {
  uint8_t last = p[plen - XorInt(1)];
  if (last > 0 && last <= XorInt(16)) {
    for (int x = 2; x <= last; x++) {
      if (p[plen - x] != last) {
        return plen;
      }
    }
    return plen - last;
  }

  return plen;
}

std::string Aesdec(const std::vector<uint8_t> &data, const uint8_t *key,
                   const uint8_t *iv) {
    size_t l = data.size();
    std::unique_ptr<uint8_t[]> encrypt(new uint8_t[l]);
    memcpy(encrypt.get(), data.data(), l);

    AES_ctx ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, encrypt.get(), l);

    std::unique_ptr<uint8_t[]> out(new uint8_t[l + 1]);
    memcpy(out.get(), encrypt.get(), l);
    out[l] = 0;

    int realLength = Pkcs7cut(out.get(), l);
    out[realLength] = 0;

    std::string res = reinterpret_cast<char *>(out.get());
    return res;
  /*

  // Original code:

  size_t l = data.size();
  uint8_t *encrypt = new uint8_t[l];
  memcpy(encrypt, data.data(), l);

  struct AES_ctx ctx;
  AES_init_ctx_iv(&ctx, key, iv);
  AES_CBC_decrypt_buffer(&ctx, encrypt, l);

  uint8_t *out = new uint8_t[l + 1];
  memcpy(out, encrypt, l);
  out[l] = XorInt(0);

  int realLength = Pkcs7cut(out, l);
  out[realLength] = XorInt(0);

  delete[] encrypt;
  std::string res = reinterpret_cast<char *>(out);

  delete[] out;
  return res;
  */
}

std::string Decrypt(const std::string &base64) {
  size_t buflen = base64_decode(base64.c_str(), base64.length(), nullptr);
  if (buflen == XorInt(0)) {
    return xorstr_("");
  }

  std::vector<uint8_t> buf(buflen);
  base64_decode(base64.c_str(), base64.length(), &buf[0]);

  std::vector<uint8_t> iv(buf.begin(), buf.begin() + XorInt(16));
  std::vector<uint8_t> data(buf.begin() + XorInt(16), buf.end());

  return Aesdec(data, GetKey(), iv.data());
}

Napi::Value ModulePrototypeCompile(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  AddonData *addon_data = static_cast<AddonData *>(info.Data());
  Napi::String content = info[0].As<Napi::String>();
  Napi::String filename = info[1].As<Napi::String>();
  std::string filename_str = filename.Utf8Value();
  Napi::Function old_compile =
      addon_data->functions[FN_MODULE_PROTOTYPE__COMPILE].Value();

  if (filename_str.find(xorstr_("app.asar")) != std::string::npos) {
    return old_compile.Call(
        info.This(),
        {Napi::String::New(env, Decrypt(content.Utf8Value())), filename});
  }
  return old_compile.Call(info.This(), {content, filename});
}

Napi::Function MakeRequireFunction(Napi::Env *env, const Napi::Object &mod) {
  Napi::Function make_require =
      env->RunScript(scriptRequire).As<Napi::Function>();
  return make_require({mod}).As<Napi::Function>();
}

Napi::Value GetModuleObject(Napi::Env *env, const Napi::Object &main_module,
                            const Napi::Object &this_exports) {
  Napi::Function find_function =
      env->RunScript(scriptFind).As<Napi::Function>();
  Napi::Value res = find_function({main_module, this_exports});
  if (res.IsNull()) {
    Napi::Error::New(*env, xorstr_("Cannot find module object."))
        .ThrowAsJavaScriptException();
  }
  return res;
}

void ShowErrorAndQuit(const Napi::Env &env, const Napi::Object &electron,
                      const Napi::String &message) {
  Napi::Value ELECTRON_RUN_AS_NODE = env.Global()
                                         .As<Napi::Object>()
                                         .Get(xorstr_("process"))
                                         .As<Napi::Object>()
                                         .Get(xorstr_("env"))
                                         .As<Napi::Object>()
                                         .Get(xorstr_("ELECTRON_RUN_AS_NODE"));

  if (!ELECTRON_RUN_AS_NODE.IsUndefined() &&
      ELECTRON_RUN_AS_NODE != Napi::Number::New(env, XorInt(0)) &&
      ELECTRON_RUN_AS_NODE != Napi::String::New(env, xorstr_(""))) {
    ConsoleError(env, message);
    exit(1);
  } else {
    Napi::Object dialog = electron.Get(xorstr_("dialog")).As<Napi::Object>();
    dialog.Get(xorstr_("showErrorBox"))
        .As<Napi::Function>()
        .Call(dialog, {Napi::String::New(env, xorstr_("Error")), message});

    Napi::Object app = electron.Get(xorstr_("app")).As<Napi::Object>();
    Napi::Function quit = app.Get(xorstr_("quit")).As<Napi::Function>();
    quit.Call(app, {});
  }
}

// This doesn't compile very well on Linux right now, so removing until I can find a way to make the Windows headers only run on Windows

/*
// INT 3 is from
// https://github.com/LordNoteworthy/al-khaser/blob/master/al-khaser/AntiDebug/Interrupt_3.cpp

/*
INT 3 generates a call to trap in the debugger and is triggered by opcode 0xCC
within the executing process. When a debugger is attached, the 0xCC execution
will cause the debugger to catch the breakpoint and handle the resulting
exception. If a debugger is not attached, the exception is passed through to a
structured exception handler thus informing the process that no debugger is
present. Vectored Exception Handling is used here because SEH is an anti-debug
trick in itself.
* /

static BOOL SwallowedException = TRUE;

static LONG CALLBACK VectoredHandler(_In_ PEXCEPTION_POINTERS ExceptionInfo) {
  SwallowedException = FALSE;
  if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_BREAKPOINT) {
    // Increase EIP/RIP to continue execution.
#ifdef _WIN64
    ExceptionInfo->ContextRecord->Rip++;
#else
    ExceptionInfo->ContextRecord->Eip++;
#endif
    return EXCEPTION_CONTINUE_EXECUTION;
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

BOOL Interrupt_3() {
  PVOID Handle = AddVectoredExceptionHandler(1, VectoredHandler);
  SwallowedException = TRUE;
  __debugbreak();
  RemoveVectoredExceptionHandler(Handle);
  return SwallowedException;
}

bool IsThereADebugger() {
  if (Interrupt_3()) {
    return true;
  }

  if (IsDebuggerPresent()) {
    return true;
  }

  // https://anti-debug.checkpoint.com/techniques/object-handles.html#loadlibrary
  CHAR szBuffer[] = {
      "C:\\Windows\\System32\\calc.exe"}; // We'll get an error saying cannot
                                          // convert from 'char *' to 'CHAR' if
                                          // we try to use xorstr here
                                          // unfortunately
  LoadLibraryA(szBuffer);
  if (INVALID_HANDLE_VALUE ==
      CreateFileA(szBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL)) {
    return true;
  }

  __try {
    // Very simple way to detect debuggers.
    // More info:
    // https://anti-debug.checkpoint.com/techniques/misc.html#dbgprint

    RaiseException(DBG_PRINTEXCEPTION_C, 0, 0, 0);
  } __except (GetExceptionCode() == DBG_PRINTEXCEPTION_C) {
    return false;
  }

  return true;
}
*/

Napi::Object Init(Napi::Env env, Napi::Object exports) {
#ifdef _TARGET_ELECTRON_RENDERER_
  Napi::Object main_module =
      env.Global().Get(xorstr_("module")).As<Napi::Object>();
#else
  Napi::Object process =
      env.Global().Get(xorstr_("process")).As<Napi::Object>();
  Napi::Array argv = process.Get(xorstr_("argv")).As<Napi::Array>();
  for (uint32_t i = 0; i < argv.Length(); ++i) {
    std::string arg = argv.Get(i).As<Napi::String>().Utf8Value();

    if (arg.find(xorstr_("--inspect")) == XorInt(0) ||
        arg.find(xorstr_("--remote-debugging-port")) == XorInt(0)) {
      Napi::Error::New(
          env,
          debuggerDetected
      ).ThrowAsJavaScriptException();

      return exports;
    }
  }

  Napi::Object main_module =
      process.Get(xorstr_("mainModule")).As<Napi::Object>();
#endif

  if (IsThereADebugger()) {
      Napi::Error::New(
          env,
          debuggerDetected
      ).ThrowAsJavaScriptException();
  }

  if (InsideAVM()) {
    Napi::Error::New(
        env,
        virtualMachineDetected
    ).ThrowAsJavaScriptException();
  }

  Napi::Object this_module =
      GetModuleObject(&env, main_module, exports).As<Napi::Object>();
  Napi::Function require = MakeRequireFunction(&env, this_module);

  Napi::Object electron =
      require({Napi::String::New(env, xorstr_("electron"))}).As<Napi::Object>();
  Napi::Object module_constructor =
      require({Napi::String::New(env, xorstr_("module"))}).As<Napi::Object>();

  Napi::Value module_parent = this_module.Get(xorstr_("parent"));

#ifdef _TARGET_ELECTRON_RENDERER_
  if (module_parent != main_module) {
    Napi::Object ipcRenderer =
        electron.Get(xorstr_("ipcRenderer")).As<Napi::Object>();
    Napi::Function sendSync =
        ipcRenderer.Get(xorstr_("sendSync")).As<Napi::Function>();
    sendSync.Call(ipcRenderer,
                  {Napi::String::New(env, xorstr_("__SHOW_ERROR_AND_QUIT__"))});
    return exports;
  }
#else
  if (this_module != main_module ||
      (module_parent != module_constructor &&
       module_parent != env.Undefined() && module_parent != env.Null())) {
    ShowErrorAndQuit(env, electron, Napi::String::New(env, integirtyCheckFailed));
    return exports;
  }
#endif

  AddonData *addon_data = env.GetInstanceData<AddonData>();

  if (addon_data == nullptr) {
    addon_data = new AddonData();
    env.SetInstanceData(addon_data);
  }

  Napi::Object module_prototype =
      module_constructor.Get(xorstr_("prototype")).As<Napi::Object>();
  addon_data->functions[FN_MODULE_PROTOTYPE__COMPILE] = Napi::Persistent(
      module_prototype.Get(xorstr_("_compile")).As<Napi::Function>());

  module_prototype.DefineProperty(Napi::PropertyDescriptor::Function(
      env, Napi::Object::New(env), xorstr_("_compile"), ModulePrototypeCompile,
      napi_enumerable, addon_data));

#ifdef _TARGET_ELECTRON_RENDERER_
  return exports;
#else

  Napi::Value ELECTRON_RUN_AS_NODE = env.Global()
                                         .As<Napi::Object>()
                                         .Get(xorstr_("process"))
                                         .As<Napi::Object>()
                                         .Get(xorstr_("env"))
                                         .As<Napi::Object>()
                                         .Get(xorstr_("ELECTRON_RUN_AS_NODE"));

  if (ELECTRON_RUN_AS_NODE.IsUndefined() ||
      ELECTRON_RUN_AS_NODE == Napi::Number::New(env, XorInt(0)) ||
      ELECTRON_RUN_AS_NODE == Napi::String::New(env, xorstr_(""))) {
    Napi::Object ipcMain = electron.Get(xorstr_("ipcMain")).As<Napi::Object>();
    Napi::Function once = ipcMain.Get(xorstr_("once")).As<Napi::Function>();

    once.Call(
        ipcMain,
        {Napi::String::New(env, xorstr_("__SHOW_ERROR_AND_QUIT__")),
         Napi::Function::New(
             env, [](const Napi::CallbackInfo &info) -> Napi::Value {
               Napi::Env env = info.Env();
               Napi::Object event = info[0].As<Napi::Object>();
               Napi::Object mm = env.Global()
                                     .Get(xorstr_("process"))
                                     .As<Napi::Object>()
                                     .Get(xorstr_("mainModule"))
                                     .As<Napi::Object>();
               Napi::Function req =
                   mm.Get(xorstr_("require")).As<Napi::Function>();
               ShowErrorAndQuit(
                   env,
                   req.Call(mm, {Napi::String::New(env, xorstr_("electron"))})
                       .As<Napi::Object>(),
                   Napi::String::New(env, integirtyCheckFailed));
               event.Set(xorstr_("returnValue"), env.Null());
               return env.Undefined();
             })});
  }

  try {
    require({Napi::String::New(env, xorstr_("./main.js"))})
        .As<Napi::Function>()
        .Call({GetKeyArray(env)});
  } catch (const Napi::Error &e) {
    ShowErrorAndQuit(env, electron, e.Get(xorstr_("stack")).As<Napi::String>());
  }
  return exports;
#endif
}

} // namespace

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)

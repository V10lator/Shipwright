#include "SohHooks.h"
#include <map>
#include <string>
#include <vector>
#include <stdarg.h>
#include <iostream>

std::map<std::string, std::vector<HookFunc>> listeners;
HookCall call;

/*
#############################
   Module: Hook C++ Handle
#############################
*/

namespace ModInternal {

    void registerHookListener(HookListener listener) {
        listeners[listener.hookName].push_back(listener.callback);
    }

    bool handleHook(HookCall* call) {
        for (size_t l = 0; l < listeners[call->name].size(); l++) {
            (listeners[call->name][l])(call);
        }
        return call->cancelled;
    }

    void bindHook(std::string name) {
        call.name = name;
    }

    void initBindHook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                call.baseArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }
    }

    bool callBindHook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                call.hookedArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }


        const bool cancelled = handleHook(&call);

        call.name = "";
        call.baseArgs.clear();
        call.hookedArgs.clear();

        return cancelled;
    }
}

/*
#############################
    Module: Hook C Handle
#############################
*/

extern "C" {

    void bind_hook(char* name) {
        call.name = name;
    }

    void init_hook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                call.baseArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }
    }

    bool call_hook(int length, ...) {
        if (length > 0) {
            va_list args;
            va_start(args, length);
            for (int i = 0; i < length; i++) {
                HookParameter currentParam = va_arg(args, struct HookParameter);
                call.hookedArgs[currentParam.name] = currentParam.parameter;
            }
            va_end(args);
        }

        const bool cancelled = ModInternal::handleHook(&call);

        call.name = "";
        call.baseArgs.clear();
        call.hookedArgs.clear();

        return cancelled;
    }

}
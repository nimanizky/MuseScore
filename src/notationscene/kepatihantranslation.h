/* SPDX-License-Identifier: GPL-3.0-only */
#pragma once

#include "async/notification.h"

namespace mu::notation {
// Shared by all score views and their note-input bars for this application session.
class KepatihanTranslation
{
public:
    static bool enabled() { return s_enabled; }
    static void toggle()
    {
        s_enabled = !s_enabled;
        s_changed.notify();
    }
    static muse::async::Notification changed() { return s_changed; }

private:
    inline static bool s_enabled = true;
    inline static muse::async::Notification s_changed;
};
}

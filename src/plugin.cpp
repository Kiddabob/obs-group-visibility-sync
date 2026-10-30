// SPDX-License-Identifier: GPL-2.0-or-later
#include "sync-core.hpp"
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <util/platform.h>
#include <QAction>
#include <QDir>
#include <QFileInfo>
#include <QMainWindow>
#include <QPointer>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Community OBS tooling")
MODULE_EXPORT const char *obs_module_name(void) { return "Group Visibility Sync"; }
MODULE_EXPORT const char *obs_module_description(void) { return "Optionally mirror grouped source visibility to the matching Program scene."; }

namespace {
using SourceRef=std::shared_ptr<obs_source_t>;
using ItemRef=std::shared_ptr<obs_sceneitem_t>;
SourceRef hold(obs_source_t *source) { return SourceRef(obs_source_get_ref(source),obs_source_release); }

class Controller final : public QObject {
    std::atomic<bool> enabled{false}, suspended{false};
    std::mutex mutex;
    std::vector<obs_weak_source_t *> groups;
    QAction *action=nullptr;
    bool connected=false, finished=false;

    static void created(void *context,calldata_t *data) {
        auto *self=static_cast<Controller *>(context);
        auto *source=static_cast<obs_source_t *>(calldata_ptr(data,"source"));
        self->watch(source);
    }
    static void visibility(void *context,calldata_t *data) {
        auto *self=static_cast<Controller *>(context);
        if (!self->enabled.load() || self->suspended.load()) return;
        auto *scene=static_cast<obs_scene_t *>(calldata_ptr(data,"scene"));
        auto *item=static_cast<obs_sceneitem_t *>(calldata_ptr(data,"item"));
        if (!scene || !item) return;
        obs_source_t *group=obs_scene_get_source(scene);
        obs_source_t *source=obs_sceneitem_get_source(item);
        if (!group || !source || !obs_source_is_group(group) || obs_obj_is_private(group)) return;
        SourceRef group_ref=hold(group), source_ref=hold(source);
        if (!group_ref || !source_ref) return;
        obs_sceneitem_addref(item);
        ItemRef item_ref(item,obs_sceneitem_release);
        const bool visible=calldata_bool(data,"visible");
        QMetaObject::invokeMethod(self,[self,group_ref,source_ref,item_ref,visible] {
            if (!self->enabled.load() || self->suspended.load() || self->finished) return;
            if (obs_source_removed(group_ref.get()) || obs_source_removed(source_ref.get()) ||
                obs_sceneitem_get_scene(item_ref.get())!=obs_group_from_source(group_ref.get()) ||
                obs_sceneitem_visible(item_ref.get())!=visible ||
                !obs_frontend_preview_program_mode_active()) return;
            obs_source_t *transition=obs_get_output_source(0);
            if (!transition) return;
            if (obs_source_get_type(transition)!=OBS_SOURCE_TYPE_TRANSITION) {
                obs_source_release(transition); return;
            }
            std::vector<obs_source_t *> roots;
            if (obs_transition_is_active(transition)) {
                roots.push_back(obs_transition_get_source(transition,OBS_TRANSITION_SOURCE_A));
                roots.push_back(obs_transition_get_source(transition,OBS_TRANSITION_SOURCE_B));
            } else {
                roots.push_back(obs_transition_get_active_source(transition));
            }
            for (auto *root:roots) {
                if (root) {
                    auto result=sync_group_visibility(group_ref.get(),source_ref.get(),visible,root);
                    if (result.ambiguous_groups)
                        blog(LOG_WARNING,"[group-visibility-sync] Ambiguous repeated source in a group; live update skipped.");
                    obs_source_release(root);
                }
            }
            obs_source_release(transition);
        },Qt::QueuedConnection);
    }
    void watch(obs_source_t *source) {
        if (!source || !obs_source_is_group(source) || obs_obj_is_private(source) || obs_source_removed(source)) return;
        std::lock_guard<std::mutex> lock(mutex);
        for (auto it=groups.begin();it!=groups.end();) {
            if (obs_weak_source_expired(*it)) { obs_weak_source_release(*it); it=groups.erase(it); }
            else {
                if (obs_weak_source_references_source(*it,source)) return;
                ++it;
            }
        }
        auto *weak=obs_source_get_weak_source(source);
        if (!weak) return;
        signal_handler_connect(obs_source_get_signal_handler(source),"item_visible",visibility,this);
        groups.push_back(weak);
    }
    void watchAll() {
        obs_enum_sources([](void *context,obs_source_t *source){
            static_cast<Controller *>(context)->watch(source); return true;
        },this);
    }
    static void event(enum obs_frontend_event e,void *context) {
        auto *self=static_cast<Controller *>(context);
        if (e==OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGING) self->suspended.store(true);
        else if (e==OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED || e==OBS_FRONTEND_EVENT_FINISHED_LOADING) {
            self->watchAll(); self->suspended.store(false);
        } else if (e==OBS_FRONTEND_EVENT_EXIT) {
            self->suspended.store(true); self->enabled.store(false);
        }
    }
    QString configPath() const {
        char *path=obs_module_config_path("settings.json");
        QString result=QString::fromUtf8(path?path:""); bfree(path); return result;
    }
    bool loadSetting() const {
        const auto path=configPath().toUtf8();
        obs_data_t *data=obs_data_create_from_json_file_safe(path.constData(),"bak");
        bool value=data && obs_data_get_bool(data,"enabled");
        obs_data_release(data);
        return value;
    }
    void saveSetting(bool value) const {
        const auto file=configPath();
        if (file.isEmpty() || !QDir().mkpath(QFileInfo(file).absolutePath())) return;
        obs_data_t *data=obs_data_create();
        obs_data_set_bool(data,"enabled",value);
        if (!obs_data_save_json_safe(data,file.toUtf8().constData(),"tmp","bak"))
            blog(LOG_WARNING,"[group-visibility-sync] Could not save the enable setting.");
        obs_data_release(data);
    }
public:
    explicit Controller(QMainWindow *main):QObject(main) {
        action=static_cast<QAction *>(obs_frontend_add_tools_menu_qaction("Sync grouped visibility to Program"));
        if (action) {
            action->setCheckable(true);
            action->setToolTip("Mirror show/hide changes inside matching groups to the live Program scene.");
            const bool saved=loadSetting(); enabled.store(saved); action->setChecked(saved);
            connect(action,&QAction::toggled,this,[this](bool checked){enabled.store(checked);saveSetting(checked);});
        }
        signal_handler_connect(obs_get_signal_handler(),"source_create",created,this);
        obs_frontend_add_event_callback(event,this);
        connected=true;
        watchAll();
    }
    ~Controller() override {
        finished=true; enabled.store(false); suspended.store(true);
        if (connected) {
            signal_handler_disconnect(obs_get_signal_handler(),"source_create",created,this);
            obs_frontend_remove_event_callback(event,this);
        }
        std::lock_guard<std::mutex> lock(mutex);
        for (auto *weak:groups) {
            auto *source=obs_weak_source_get_source(weak);
            if (source) {
                signal_handler_disconnect(obs_source_get_signal_handler(source),"item_visible",visibility,this);
                obs_source_release(source);
            }
            obs_weak_source_release(weak);
        }
        groups.clear();
        delete action; action=nullptr;
    }
};
QPointer<Controller> controller;
}
bool obs_module_load(void) {
    auto *main=static_cast<QMainWindow *>(obs_frontend_get_main_window());
    if (!main) { blog(LOG_WARNING,"[group-visibility-sync] OBS frontend required."); return false; }
    controller=new Controller(main);
    blog(LOG_INFO,"[group-visibility-sync] Test build 0.1.0 loaded; visibility sync is opt-in.");
    return true;
}
void obs_module_unload(void) { delete controller.data(); controller=nullptr; }

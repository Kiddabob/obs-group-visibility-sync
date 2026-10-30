// SPDX-License-Identifier: GPL-2.0-or-later
#include "sync-core.hpp"
#include <cstring>
#include <memory>
#include <unordered_set>
#include <vector>

namespace {
using SourceRef=std::shared_ptr<obs_source_t>;
using ItemRef=std::shared_ptr<obs_sceneitem_t>;
SourceRef ref(obs_source_t *s) { return SourceRef(obs_source_get_ref(s),obs_source_release); }
std::vector<ItemRef> items(obs_scene_t *scene) {
    std::vector<ItemRef> out;
    if (!scene) return out;
    obs_scene_enum_items(scene, [](obs_scene_t *,obs_sceneitem_t *item,void *ctx) {
        obs_sceneitem_addref(item);
        static_cast<std::vector<ItemRef> *>(ctx)->emplace_back(item,obs_sceneitem_release);
        return true;
    },&out);
    return out;
}
}
GroupSyncResult sync_group_visibility(obs_source_t *original_group, obs_source_t *source,
                                      bool visible, obs_source_t *program_root) {
    GroupSyncResult result;
    if (!original_group || !source || !program_root || obs_source_removed(original_group) ||
        obs_source_removed(source)) return result;
    const char *name=obs_source_get_name(original_group);
    if (!name || !*name) return result;
    std::vector<SourceRef> queue; queue.push_back(ref(program_root));
    std::unordered_set<obs_source_t *> visited;
    std::vector<ItemRef> updates;
    for (size_t index=0;index<queue.size() && index<4096;++index) {
        obs_source_t *owner=queue[index].get();
        if (!owner || !visited.insert(owner).second) continue;
        obs_scene_t *scene=obs_scene_from_source(owner);
        if (!scene) scene=obs_group_from_source(owner);
        if (!scene) continue;
        auto children=items(scene);
        const bool group=obs_source_is_group(owner);
        const char *candidate_name=obs_source_get_name(owner);
        if (group && owner!=original_group && candidate_name && std::strcmp(name,candidate_name)==0) {
            std::vector<ItemRef> matches;
            for (auto &item:children)
                if (obs_sceneitem_get_source(item.get())==source) matches.push_back(item);
            if (matches.size()==1) { ++result.matched_groups; updates.push_back(matches.front()); }
            else if (matches.size()>1) ++result.ambiguous_groups;
        }
        for (auto &item:children) {
            obs_source_t *child=obs_sceneitem_get_source(item.get());
            if (child && (obs_sceneitem_is_group(item.get()) || obs_scene_from_source(child)))
                queue.push_back(ref(child));
        }
    }
    // Enumeration locks have all been released before changing visibility.
    std::unordered_set<obs_sceneitem_t *> done;
    for (auto &item:updates)
        if (done.insert(item.get()).second && obs_sceneitem_visible(item.get())!=visible &&
            obs_sceneitem_set_visible(item.get(),visible)) ++result.changed_items;
    return result;
}

// SPDX-License-Identifier: GPL-2.0-or-later
#include "../src/sync-core.hpp"
#include <obs-module.h>
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
static const char *name(void *){return "Synthetic";}
static void *create(obs_data_t *,obs_source_t *s){return s;}
static void destroy(void *){}
static uint32_t size(void *){return 100;}
static bool first(obs_scene_t *,obs_sceneitem_t *item,void *out){*static_cast<obs_sceneitem_t **>(out)=item;return false;}
static obs_sceneitem_t *child(obs_sceneitem_t *group) {
    obs_sceneitem_t *out=nullptr; obs_scene_enum_items(obs_sceneitem_group_get_scene(group),first,&out); return out;
}
int main(){
    CHECK(obs_startup("en-US",nullptr,nullptr));
    const char *resources=std::getenv("OBS_PROBE_DATA"),*gfx=std::getenv("OBS_PROBE_GFX");CHECK(resources&&gfx);
    obs_add_data_path(resources);
    obs_audio_info ai{};ai.samples_per_sec=48000;ai.speakers=SPEAKERS_STEREO;CHECK(obs_reset_audio(&ai));
    obs_video_info vi{};vi.graphics_module=gfx;vi.fps_num=10;vi.fps_den=1;vi.base_width=320;vi.base_height=180;
    vi.output_width=320;vi.output_height=180;vi.output_format=VIDEO_FORMAT_RGBA;vi.colorspace=VIDEO_CS_709;
    vi.range=VIDEO_RANGE_FULL;vi.scale_type=OBS_SCALE_BILINEAR;CHECK(obs_reset_video(&vi)==OBS_VIDEO_SUCCESS);
    auto *private_data=obs_get_private_data();obs_data_set_bool(private_data,"AbsoluteCoordinates",true);obs_data_release(private_data);
    obs_source_info info{};info.id="group-sync-synthetic";info.type=OBS_SOURCE_TYPE_INPUT;info.output_flags=OBS_SOURCE_VIDEO;
    info.get_name=name;info.create=create;info.destroy=destroy;info.get_width=size;info.get_height=size;obs_register_source(&info);
    obs_source_t *visual=obs_source_create(info.id,"Shared visual",nullptr,nullptr);CHECK(visual);
    obs_scene_t *a=obs_scene_create("A"),*b=obs_scene_create("B");CHECK(a&&b);
    obs_sceneitem_t *one=obs_scene_add(a,visual);CHECK(one);
    obs_sceneitem_t *group=obs_scene_insert_group(a,"Shared Group",&one,1);CHECK(group);
    auto *original_group=obs_sceneitem_get_source(group);auto *original_group_ref=obs_source_get_ref(original_group);
    auto *preview_child=child(group);CHECK(preview_child);
    auto *other_item=obs_scene_add(a,visual);CHECK(other_item);
    auto *other_group=obs_scene_insert_group(a,"Other Group",&other_item,1);CHECK(other_group);
    auto *other_group_ref=obs_source_get_ref(obs_sceneitem_get_source(other_group));
    auto *top_level=obs_scene_add(a,visual);CHECK(top_level);
    CHECK(obs_scene_add(b,original_group));
    obs_scene_t *same=obs_scene_duplicate(a,"Program A",OBS_SCENE_DUP_PRIVATE_REFS);
    obs_scene_t *different=obs_scene_duplicate(b,"Program B",OBS_SCENE_DUP_PRIVATE_REFS);CHECK(same&&different);
    auto *same_group=obs_scene_find_source(same,"Shared Group"),*different_group=obs_scene_find_source(different,"Shared Group");
    auto *same_other=obs_scene_find_source(same,"Other Group");CHECK(same_group&&different_group&&same_other);
    auto *same_child=child(same_group),*different_child=child(different_group),*other_child=child(same_other);
    auto *top_copy=obs_scene_find_source(same,"Shared visual");CHECK(same_child&&different_child&&other_child&&top_copy);
    CHECK(obs_sceneitem_set_visible(preview_child,false));
    CHECK(obs_sceneitem_visible(same_child)&&obs_sceneitem_visible(different_child));
    auto r=sync_group_visibility(original_group,visual,false,obs_scene_get_source(same));
    CHECK(r.matched_groups==1&&r.changed_items==1&&!obs_sceneitem_visible(same_child));
    CHECK(obs_sceneitem_visible(other_child)&&obs_sceneitem_visible(top_copy));
    r=sync_group_visibility(original_group,visual,false,obs_scene_get_source(different));
    CHECK(r.matched_groups==1&&r.changed_items==1&&!obs_sceneitem_visible(different_child));
    CHECK(obs_sceneitem_set_visible(preview_child,true));
    CHECK(sync_group_visibility(original_group,visual,true,obs_scene_get_source(same)).changed_items==1);
    CHECK(sync_group_visibility(original_group,visual,true,obs_scene_get_source(different)).changed_items==1);
    CHECK(obs_sceneitem_visible(same_child)&&obs_sceneitem_visible(different_child));
    std::puts("PASS: hide/show in shared group reaches same and different Program scenes; unrelated items stay unchanged");
    auto *group_scene=obs_sceneitem_group_get_scene(group);
    CHECK(obs_scene_add(group_scene,visual));
    obs_scene_t *ambiguous=obs_scene_duplicate(a,"Ambiguous",OBS_SCENE_DUP_PRIVATE_REFS);CHECK(ambiguous);
    r=sync_group_visibility(original_group,visual,false,obs_scene_get_source(ambiguous));
    CHECK(r.ambiguous_groups==1&&r.changed_items==0);
    std::puts("PASS: repeated identical sources in one group are left untouched rather than matched incorrectly");
    obs_source_set_name(original_group,"Renamed shared group");
    r=sync_group_visibility(original_group,visual,false,obs_scene_get_source(same));
    CHECK(!r.matched_groups&&!r.changed_items&&obs_sceneitem_visible(same_child));
    std::puts("PASS: stale renamed group is left untouched until a new transition");
    obs_source_remove(original_group_ref);obs_source_remove(other_group_ref);
    obs_source_remove(obs_scene_get_source(a));obs_source_remove(obs_scene_get_source(b));obs_source_remove(visual);
    obs_scene_release(same);obs_scene_release(different);obs_scene_release(ambiguous);obs_scene_release(a);obs_scene_release(b);
    obs_source_release(original_group_ref);obs_source_release(other_group_ref);obs_source_release(visual);
    obs_wait_for_destroy_queue();obs_shutdown();
    std::puts("PASS: isolated scenes and sources cleaned up");
}

// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once
#include <obs.h>
struct GroupSyncResult {
    unsigned matched_groups = 0;
    unsigned changed_items = 0;
    unsigned ambiguous_groups = 0;
};
// Only matching groups and the exact referenced source are affected.
GroupSyncResult sync_group_visibility(obs_source_t *original_group, obs_source_t *source,
                                      bool visible, obs_source_t *program_root);

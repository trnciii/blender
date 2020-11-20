/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup bke
 */

#include <string.h>

#include "BLI_listbase.h"
#include "BLI_string.h"
#include "BLI_string_utils.h"
#include "BLI_utildefines.h"

#include "BKE_asset.h"
#include "BKE_icons.h"

#include "DNA_ID.h"
#include "DNA_asset_types.h"
#include "DNA_defaults.h"

#include "BLO_read_write.h"

#include "MEM_guardedalloc.h"

#ifdef WITH_ASSET_REPO_INFO
AssetRepositoryInfo *G_asset_repository_info;

AssetRepositoryInfo *BKE_asset_repository_info_global_ensure(void)
{
  if (!G_asset_repository_info) {
    G_asset_repository_info = MEM_callocN(sizeof(*G_asset_repository_info), __func__);
  }

  return G_asset_repository_info;
}

void BKE_asset_repository_info_free(AssetRepositoryInfo **repository_info)
{
  if (!*repository_info) {
    return;
  }

  LISTBASE_FOREACH_MUTABLE (AssetCatalog *, catalog, &(*repository_info)->catalogs) {
    BLI_remlink(&(*repository_info)->catalogs, catalog);
    BKE_asset_repository_catalog_free(catalog);
  }
  MEM_SAFE_FREE(*repository_info);
}

void BKE_asset_repository_info_global_free(void)
{
  BKE_asset_repository_info_free(&G_asset_repository_info);
}

void BKE_asset_repository_info_update_for_file_read(AssetRepositoryInfo **old_repository_info)
{
  /* Force recreation of the repository info. */
  BKE_asset_repository_info_free(old_repository_info);
}

#endif

AssetCatalog *BKE_asset_repository_catalog_create(const char *name)
{
  AssetCatalog *catalog = MEM_callocN(sizeof(*catalog), __func__);

  BLI_strncpy(catalog->name, name, sizeof(catalog->name));
  return catalog;
}

void BKE_asset_repository_catalog_free(AssetCatalog *catalog)
{
  MEM_freeN(catalog);
}

AssetData *BKE_asset_data_create(void)
{
  AssetData *asset_data = MEM_callocN(sizeof(AssetData), __func__);
  memcpy(asset_data, DNA_struct_default_get(AssetData), sizeof(*asset_data));
  return asset_data;
}

void BKE_asset_data_free(AssetData *asset_data)
{
  MEM_SAFE_FREE(asset_data->description);
  BLI_freelistN(&asset_data->tags);

  MEM_SAFE_FREE(asset_data);
}

static CustomTag *assetdata_tag_create(const char *const name)
{
  CustomTag *tag = MEM_callocN(sizeof(*tag), __func__);
  BLI_strncpy(tag->name, name, sizeof(tag->name));
  return tag;
}

CustomTag *BKE_assetdata_tag_add(AssetData *asset_data, const char *name)
{
  CustomTag *tag = assetdata_tag_create(name);

  BLI_addtail(&asset_data->tags, tag);
  BLI_uniquename(&asset_data->tags, tag, name, '.', offsetof(CustomTag, name), sizeof(tag->name));

  return tag;
}

/**
 * Make sure there is a tag with name \a name, create one if needed.
 */
struct CustomTagEnsureResult BKE_assetdata_tag_ensure(AssetData *asset_data, const char *name)
{
  struct CustomTagEnsureResult result = {.tag = NULL};
  if (!name[0]) {
    return result;
  }

  CustomTag *tag = BLI_findstring(&asset_data->tags, name, offsetof(CustomTag, name));

  if (tag) {
    result.tag = tag;
    result.is_new = false;
    return result;
  }

  tag = assetdata_tag_create(name);
  BLI_addtail(&asset_data->tags, tag);

  result.tag = tag;
  result.is_new = true;
  return result;
}

void BKE_assetdata_tag_remove(AssetData *asset_data, CustomTag *tag)
{
  BLI_freelinkN(&asset_data->tags, tag);
}

/* Queries -------------------------------------------- */

PreviewImage *BKE_assetdata_preview_get_from_id(const AssetData *UNUSED(asset_data), const ID *id)
{
  return BKE_previewimg_id_get(id);
}

/* .blend file API -------------------------------------------- */

void BKE_assetdata_write(BlendWriter *writer, AssetData *asset_data)
{
  BLO_write_struct(writer, AssetData, asset_data);

  if (asset_data->description) {
    BLO_write_string(writer, asset_data->description);
  }
  LISTBASE_FOREACH (CustomTag *, tag, &asset_data->tags) {
    BLO_write_struct(writer, CustomTag, tag);
  }
}

void BKE_assetdata_read(BlendDataReader *reader, AssetData *asset_data)
{
  /* asset_data itself has been read already. */

  BLO_read_data_address(reader, &asset_data->description);
  BLO_read_list(reader, &asset_data->tags);
}

# 资源来源

## 角色与动画

主角"大力士胖橘"（`Content/Sokoban/Characters/GingerCat`）是本项目自制的：模型、骨骼、调色板贴图和 Idle / Walk / Push / Cheer 四个动作都由 Blender 脚本生成，没有使用第三方角色资源。

## 场景

| 资源 | 作者 | 授权 | 在项目中的位置 |
|---|---|---|---|
| Stylized Nature MegaKit（岩石、卵石、灌木、草、花、蘑菇、树及其贴图，地形噪声贴图） | Quaternius（quaternius.com） | CC0 1.0 | `Content/Sokoban/Environment` |
| Medieval Village MegaKit（庭院石砖、圆塔尖顶、藤蔓、碎砖、木箱及其贴图） | Quaternius | CC0 1.0 | `Content/Sokoban/Environment` |

CC0 不要求署名，这里仍然列出以示感谢。

## 项目自制

- 主角：见上文"角色与动画"。
- 评分星星：`Content/Sokoban/UI` 下的 T_StarFull、T_StarEmpty、T_StarGlow，由脚本绘制。
- 材质：`Content/Sokoban/Materials` 下的 M_SokobanCharacter、M_SokobanProp、M_SokobanFoliage、M_SokobanTerrain、M_SokobanWater、M_SokobanGoalPlate、M_SokobanSwitchPlate（机关压板）、M_SokobanGlowInlay（活板门符文）、M_SokobanToon（卡通后期）、M_SokobanCloudShadows（云影灯光函数）。
- 水面波纹法线贴图 T_WaterRipples_N：由脚本按正弦波叠加生成。
- 音乐与音效：`Content/Sokoban/Audio` 下的全部声音（标题曲、关卡曲、海浪环境声和 13 个音效）由脚本合成：拨弦用 Karplus-Strong 物理模型，马林巴、拇指琴、钟和木头敲击用模态合成，海浪、摩擦和吱呀声用整形噪声，再加人性化的节奏力度和卷积混响。曲子是为本作写的。

## 引擎自带内容

基础几何体（`/Engine/BasicShapes`）和 Roboto 字体（含中文回退字体 DroidSansFallback）随 Unreal Engine 提供。

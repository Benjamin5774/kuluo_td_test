# 资源来源

## 角色与动画

主角"大力士胖橘"（`Content/Sokoban/Characters/GingerCat`）是本项目自制的：模型、骨骼、调色板贴图和 Idle / Walk / Push / Cheer 四个动作都由 Blender 脚本生成，没有使用第三方角色资源。

## 场景

| 资源 | 作者 | 授权 | 在项目中的位置 |
|---|---|---|---|
| Stylized Nature MegaKit（岩石、卵石、灌木、草、花、蘑菇、树及其贴图，地形噪声贴图） | Quaternius（quaternius.com） | CC0 1.0 | `Content/Sokoban/Environment` |
| Medieval Village MegaKit（庭院石砖、圆塔尖顶、藤蔓、碎砖、木箱、城门和门框及其贴图） | Quaternius | CC0 1.0 | `Content/Sokoban/Environment` |

CC0 不要求署名，这里仍然列出以示感谢。

## 字体

| 资源 | 作者 | 授权 | 在项目中的位置 |
|---|---|---|---|
| 海岛黑体 Haidao Sans（Medium、Bold、Heavy 三个字重） | 由思源黑体 Source Han Sans SC（Adobe）裁剪而来：只保留常用汉字（GB2312 一级）、ASCII、常用标点和游戏里用到的全部字符，并按 OFL 的要求改了名 | SIL Open Font License 1.1，许可证全文和版权声明见 `Licenses/HaidaoSans-OFL.txt` | `Content/Sokoban/UI/Fonts` |

字体里没有的字（比如关卡名里输入的生僻字）会退回到引擎自带的 DroidSansFallback。

## 项目自制

- 主角：见上文"角色与动画"。
- 评分星星：`Content/Sokoban/UI` 下的 T_StarFull、T_StarEmpty、T_StarGlow，由脚本绘制。
- 界面皮肤与图标：`Content/Sokoban/UI` 下的 T_UI*（面板、彩带标题、按钮、卡片、胶囊、圆章、暗角）、T_Icon*（猫爪、奖杯、锁、暂停）和 T_Brush*（编辑器八种笔刷），由脚本绘制。
- 材质：`Content/Sokoban/Materials` 下的 M_SokobanCharacter、M_SokobanProp、M_SokobanFoliage、M_SokobanTerrain、M_SokobanWater、M_SokobanGoalPlate、M_SokobanSciFiPad（机关和机关方块的底座）、M_SokobanEnergyField（机关方块的蓝色能量体）、M_SokobanToon（卡通后期）、M_SokobanCloudShadows（云影灯光函数）。
- 水面波纹法线贴图 T_WaterRipples_N：由脚本按正弦波叠加生成。
- 音乐与音效：`Content/Sokoban/Audio` 下的全部声音（标题曲、关卡曲、海浪环境声和 13 个音效）由脚本合成：拨弦用 Karplus-Strong 物理模型，马林巴、拇指琴、钟和木头敲击用模态合成，海浪和摩擦声用整形噪声，机关的科幻音效用滑音、颤音和玻璃质感的泛音，再加人性化的节奏力度和卷积混响。曲子是为本作写的。

## 引擎自带内容

基础几何体（`/Engine/BasicShapes`）和 DroidSansFallback（字体回退）随 Unreal Engine 提供。

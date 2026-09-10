# pfbneo / pEMU PS Vita 零侵入式汉化使用与维护手册

本工程针对 PS Vita 硬件特性实现了**零侵入式、零帧率损耗、上游更新秒级复用**的汉化方案。

---

## 一、 架构与交付文件一览

| 文件路径 | 作用 |
| :--- | :--- |
| `src/skeleton/i18n.h` | 国际化查表模块头文件（单例，基于 `std::unordered_map` O(1) 查找） |
| `src/skeleton/i18n.cpp` | 国际化核心实现，内置默认中文，支持优先从外部/romfs 加载语言包与游戏名表 |
| `data/common/romfs/lang/zh_CN.lang` | 全量简体中文语言包（涵盖全部菜单、设置组、按键操作、存档等提示） |
| `data/common/romfs/titles.csv` | 经典街机游戏中文名映射表（KOF、合金弹头、街霸、三国志、西游、名将等数百款大作） |
| `.github/workflows/vita-release.yml` | 优化后的 Vita CI/CD 自动构建工作流（支持生成 Artifact 下载） |

---

## 二、 PS Vita 上的两种使用途径

### 途径 1：无需重新编译，外部目录热覆盖（适合已安装官方原版）
pEMU 原生具备外部数据优先查找机制，即使使用官方原版 VPK，也可直接外部热挂载中文资产：
1. **中文字体**：
   - 准备一个精简中文字体（推荐 3500 常用字版，约 1.5MB~2.5MB，如思源黑体精简版、得意黑等）；
   - 重命名为 `default.ttf`；
   - 通过 VitaShell 放入 PS Vita 存储卡的 `ux0:data/pfbneo/skins/default/default.ttf`。
2. **语言包与游戏名表**：
   - 将 `zh_CN.lang` 放入 `ux0:data/pfbneo/lang/zh_CN.lang`；
   - 将 `titles.csv` 放入 `ux0:data/pfbneo/titles.csv`。

---

### 途径 2：使用 GitHub Actions 自动云端编译（推荐，最省心）
本项目无需本地搭建复杂的 Linux + VitaSDK 交叉编译工具链：
1. 将当前仓库推送到您的 GitHub；
2. 进入仓库页面的 **Actions** 标签页；
3. 在左侧选择 **`vita-release`** 工作流；
4. 点击右侧的 **Run workflow** 按钮；
5. 构建完成后，在任务页面的 **Artifacts** 区域即可直接下载编译打包好的 **`pfbneo.vpk`**；
6. 传输到 PS Vita 上用 VitaShell 点击安装即可享受到开箱即用的中文界面！

---

## 三、 游戏列表中文名的扩展与自定义

`titles.csv` 采用极简的 CSV 格式：
```csv
# zipROM文件名, 中文展示名
kof97,拳皇 97
mslug3,合金弹头 3
dino,恐龙快打
```
- **如何增加游戏**：用任意文本编辑器打开 `titles.csv`，在末尾添加您喜欢的游戏即可；
- **优先级机制**：如果游戏中包含在 `titles.csv` 中，优先显示中文译名；若未配置，则自动回退为 FBNeo 默认英文全称，保证 100% 兼容。

---

## 四、 后期官方上游更新时的“秒级同步维护法”

当官方原作者（Cpasjuste/pemu）发布新代码或修复 BUG 时：

```bash
# 1. 添加并拉取官方上游更新
git remote add upstream https://github.com/Cpasjuste/pemu.git
git fetch upstream

# 2. 将当前汉化分支变基（Rebase）到最新上游
git rebase upstream/master
```

### 为什么本方案不会产生代码冲突？
1. **核心翻译逻辑独立**：所有的查表与解析都在独立的 `i18n.h` / `i18n.cpp` 中，不修改上游原业务逻辑。
2. **语言包与游戏名纯文本化**：修改只存在于 `data/.../lang/` 和 `titles.csv`。
3. **极简挂钩点**：只在 UI 显示文字的终点做了轻量级查表，`git rebase` 可以近乎 100% 自动合并成功。
4. 变基完成后，直接推送至 GitHub，Actions 会重新自动为您生成最新版本的中文 `pfbneo.vpk`。

# UMG Runtime Reflector

UMG Runtime Reflector 是一个 UE4 Editor 插件，用于在 PIE 运行时查看和调试当前游戏画面中的 UMG 对象树。

它不是官方 Widget Reflector 的替代品。官方工具会展示大量 Slate 内部层级，而本插件只关注业务侧对象：

- `UUserWidget`
- `UWidget`

这样可以在运行时调试 UI 时更直接地定位蓝图控件、查看属性、调整布局和观察表现。

## 功能概览

- 在 Editor 菜单中打开独立 Dock 面板。
- 左侧显示运行时 UMG 对象树。
- 右侧显示标准 UE `DetailsView`。
- 支持运行时编辑 `UWidget` / `UUserWidget` 属性。
- 支持搜索控件变量名 / 对象名。
- 支持树节点 CheckBox 控制控件 `Visible` / `Collapsed`。
- 支持点击类名打开对应 UI 蓝图。
- 支持实时捕获鼠标位置下的 UMG 控件。
- 支持黄色 Frame 高亮选中控件的位置和大小。
- 不捕捉 `WidgetComponent`。
- 只作为 Editor / PIE 调试工具使用，不考虑打包环境。

## 安装方式

将插件目录放入工程：

```text
YourProject/
  Plugins/
    UMGRuntimeReflector/
      UMGRuntimeReflector.uplugin
      Source/
```

启动 UE4 Editor 后，在插件管理器中确认 `UMG Runtime Reflector` 已启用。

## 使用方式

1. 启动 UE4 Editor。
2. 运行 PIE。
3. 从菜单 `Window -> UMG运行时反射器` 打开面板。
4. 在左侧对象树中选择 UMG 节点。
5. 在右侧 `DetailsView` 中查看或编辑运行时属性。

没有 PIE 运行时，左侧对象树为空。

## 面板布局

顶部工具栏包含：

- 搜索框：搜索控件变量名 / 对象名。
- 刷新：重新扫描当前 PIE 世界中的 UMG。
- 实时捕获：进入或退出鼠标位置捕获模式。
- 显示Frame：控制黄色高亮框显示，默认勾选。

左侧为 UMG 对象树，根节点显示为 `GameViewport`。

右侧为标准 `DetailsView`，用于查看和编辑当前选中对象。

## UMG 对象树规则

运行 PIE 时，插件会从当前运行世界中收集已添加到 Viewport 的 `UUserWidget`，并根据每个蓝图自身的 `WidgetTree` 构建树结构。

树节点包含：

- 可见性 CheckBox
- UUserWidget / UWidget 图标
- 对象名或变量名
- 类名链接

节点显示规则：

- `UUserWidget` 节点显示自身类名，并去除 `_C` 后缀。
- `UWidget` 节点右侧类名显示所属父级 `UUserWidget` 类名，并去除 `_C` 后缀。
- 点击类名可打开对应 UI 蓝图。
- 即使控件没有勾选 `IsVariable`，只要存在于运行时 `WidgetTree` 中，也会被展示和搜索。

## 可见性控制

每个树节点前都有 CheckBox：

- 勾选：将控件设置为 `Visible`。
- 取消勾选：将控件设置为 `Collapsed`。

这个功能用于在 PIE 运行时快速开关 UI 节点，观察布局和交互变化。

## DetailsView 属性编辑

右侧使用 UE 标准 `DetailsView`，允许直接编辑运行时对象属性。

当前支持：

- 普通 Widget 属性。
- Canvas Slot 属性。
- Horizontal / Vertical Alignment。
- SlateChildSize。
- 各类 PanelSlot 默认属性。

Canvas Slot 面板采用源码级复用 UE UMG Designer 的实现方式，在插件私有代码中维护，不直接依赖 `UMGEditor` 私有未导出符号。

Horizontal Alignment、Vertical Alignment、SlateChildSize 也注册为插件当前 `DetailsView` 实例的本地属性定制，不污染全局 PropertyEditor。

## 实时捕获

点击“实时捕获”后：

- 按钮文字变为“退出捕获”。
- 按钮高亮。
- 再次点击或按 Esc 退出捕获。

捕获规则：

- 鼠标进入游戏区域时才会捕获。
- 默认使用 Tunnel 策略捕获当前位置对应的 UMG 节点。
- 按住 Alt 时使用 Deepest 策略，捕获当前位置最深层控件。
- 捕获成功后自动在左侧树中选中节点，并刷新右侧 `DetailsView`。

## Frame 高亮规则

`显示Frame` 默认开启。

选中任意节点时：

- 如果选中 `UWidget`，绘制该 `UWidget` 自身位置和大小。
- 如果选中 `UUserWidget`，绘制该 `UUserWidget` 自身位置和大小。
- 不管它是否处于 ScrollBox、Canvas、Overlay、HorizontalBox 等容器中。
- 不管它是否被父容器裁剪。
- 只要目标自身不是 `Hidden` / `Collapsed`，就正常绘制。

Frame 通过 Viewport 顶层 Overlay 绘制。边框由四个像素对齐的矩形边组成，避免闭合折线在缩放或半像素位置下出现斜边问题。

## 技术要点

### 对象收集

插件只在 Editor / PIE 环境中工作。

对象来源：

- 遍历 `GEngine->GetWorldContexts()`。
- 过滤 `EWorldType::PIE`。
- 从 PIE 世界中收集 `IsInViewport()` 的 `UUserWidget`。

### 树构建

树结构基于 UMG 侧对象，而不是 Slate 层级。

遍历来源包括：

- `UUserWidget::WidgetTree->RootWidget`
- `UPanelWidget` 子节点
- `INamedSlotInterface` 插槽内容

这样可以覆盖常见 UMG 蓝图控件结构，同时避免展开底层 Slate 控件。

### 捕获实现

实时捕获通过 Slate 输入预处理器完成：

- 注册 `IInputProcessor`
- 监听鼠标移动和 Esc
- 使用 `FSlateApplication::LocateWindowUnderMouse`
- 判断鼠标是否在游戏 Viewport 内
- 根据策略匹配对应 UMG 对象

### Frame 绘制

Frame 通过以下方式绘制到游戏视口顶层：

- `UGameViewportClient::AddViewportWidgetContent`
- 自定义 `SLeafWidget`
- `UWidget::GetPaintSpaceGeometry()`
- `FSlateDrawElement::MakeBox`

绘制时只判断目标对象自身可见性，不处理父容器裁剪。

## 仓库提交建议

建议提交以下内容：

- `UMGRuntimeReflector.uplugin`
- `Source/`
- `README.md`
- `.gitignore`

不要提交以下生成目录：

- `Binaries/`
- `Intermediate/`
- `Saved/`
- `DerivedDataCache/`

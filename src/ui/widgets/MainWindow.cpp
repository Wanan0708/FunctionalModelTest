#include "ui/widgets/MainWindow.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <unordered_set>

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsScene>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSplitter>
#include <QTimer>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <QWidget>

namespace {

constexpr double kPi = 3.14159265358979323846;

double sensorFacingDegrees(const fm::app::EntityRenderState& state)
{
    constexpr double minimumRenderableSpeed = 1e-3;
    const double speed = std::hypot(state.velocity.x, state.velocity.y);
    if (speed > minimumRenderableSpeed) {
        return std::atan2(state.velocity.y, state.velocity.x) * 180.0 / kPi;
    }

    return state.headingDegrees;
}

bool pointFallsWithinSensorField(const fm::app::EntityRenderState& sensorState, const fm::core::Vector2& targetPosition)
{
    if (sensorState.sensorFieldOfViewDegrees >= 359.999) {
        return true;
    }

    const double dx = targetPosition.x - sensorState.position.x;
    const double dy = targetPosition.y - sensorState.position.y;
    const double distanceSquared = dx * dx + dy * dy;
    if (distanceSquared <= 1e-12) {
        return true;
    }

    const double distance = std::sqrt(distanceSquared);
    const double targetDirectionX = dx / distance;
    const double targetDirectionY = dy / distance;
    const double facingRadians = sensorFacingDegrees(sensorState) * kPi / 180.0;
    const double forwardX = std::cos(facingRadians);
    const double forwardY = std::sin(facingRadians);
    const double dotProduct = std::clamp(forwardX * targetDirectionX + forwardY * targetDirectionY, -1.0, 1.0);
    const double angleOffsetRadians = std::acos(dotProduct);
    return angleOffsetRadians <= sensorState.sensorFieldOfViewDegrees * 0.5 * kPi / 180.0;
}

class ZoomableGraphicsView final : public QGraphicsView {
public:
    explicit ZoomableGraphicsView(QWidget* parent = nullptr)
        : QGraphicsView(parent)
    {
    }

    explicit ZoomableGraphicsView(QGraphicsScene* scene, QWidget* parent = nullptr)
        : QGraphicsView(scene, parent)
    {
    }

    void setZoomCallback(std::function<void()> callback)
    {
        zoomCallback_ = std::move(callback);
    }

protected:
    void wheelEvent(QWheelEvent* event) override
    {
        if (event == nullptr || event->angleDelta().y() == 0) {
            QGraphicsView::wheelEvent(event);
            return;
        }

        const double zoomFactor = event->angleDelta().y() > 0 ? 1.15 : (1.0 / 1.15);
        if (zoomCallback_) {
            zoomCallback_();
        }
        scale(zoomFactor, zoomFactor);
        event->accept();
    }

private:
    std::function<void()> zoomCallback_;
};

QString toStateText(fm::core::SimulationState state)
{
    switch (state) {
    case fm::core::SimulationState::Stopped:
        return QStringLiteral("已停止");
    case fm::core::SimulationState::Running:
        return QStringLiteral("运行中");
    case fm::core::SimulationState::Paused:
        return QStringLiteral("已暂停");
    }

    return QStringLiteral("未知");
}

QString joinEntityMeta(const fm::app::EntityRenderState& state)
{
    QStringList parts;
    if (!state.side.empty()) {
        parts.push_back(QString::fromStdString(state.side));
    }

    if (!state.role.empty()) {
        parts.push_back(QString::fromStdString(state.role));
    }

    if (!state.category.empty()) {
        parts.push_back(QString::fromStdString(state.category));
    }

    return parts.isEmpty() ? QStringLiteral("未分类") : parts.join(QStringLiteral(" / "));
}

QString formatRouteWaypoints(const std::vector<fm::app::RouteWaypointRenderState>& routeWaypoints)
{
    if (routeWaypoints.empty()) {
        return QStringLiteral("无");
    }

    QStringList points;
    for (const auto& waypoint : routeWaypoints) {
        points.push_back(QStringLiteral("%1 (%2, %3) 停留 %4 s")
                             .arg(QString::fromStdString(waypoint.name))
                             .arg(waypoint.position.x, 0, 'f', 1)
                             .arg(waypoint.position.y, 0, 'f', 1)
                             .arg(waypoint.loiterSeconds, 0, 'f', 0));
    }

    return points.join(QStringLiteral("\n"));
}

QString formatEntityTags(const std::vector<std::string>& tags)
{
    if (tags.empty()) {
        return QStringLiteral("无");
    }

    QStringList tagTexts;
    for (const auto& tag : tags) {
        tagTexts.push_back(QString::fromStdString(tag));
    }

    return tagTexts.join(QStringLiteral(", "));
}

QString formatMissionExecutionStatus(const std::string& statusCode)
{
    if (statusCode == "unassigned") {
        return QStringLiteral("未分配");
    }

    if (statusCode == "idle") {
        return QStringLiteral("待命");
    }

    if (statusCode == "navigating") {
        return QStringLiteral("机动中");
    }

    if (statusCode == "loitering") {
        return QStringLiteral("等待中");
    }

    if (statusCode == "tracking") {
        return QStringLiteral("跟踪中");
    }

    if (statusCode == "completed") {
        return QStringLiteral("已完成");
    }

    if (statusCode == "aborted") {
        return QStringLiteral("已中止");
    }

    return QStringLiteral("未知");
}

QString formatMissionStatusBadge(const fm::app::EntityRenderState& state)
{
    if (state.missionExecutionStatus == "completed") {
        return QStringLiteral("[已完成]");
    }

    if (state.missionExecutionStatus == "aborted") {
        return QStringLiteral("[已中止]");
    }

    return QString();
}

int missionStatusSortPriority(const fm::app::EntityRenderState& state)
{
    if (state.missionExecutionStatus == "aborted") {
        return 3;
    }

    if (state.missionExecutionStatus == "completed") {
        return 2;
    }

    return 1;
}

bool matchesEntityFilter(const fm::app::EntityRenderState& state, const QString& filterKey)
{
    if (filterKey == QStringLiteral("active")) {
        return state.missionExecutionStatus != "completed" && state.missionExecutionStatus != "aborted";
    }

    if (filterKey == QStringLiteral("terminal")) {
        return state.missionExecutionStatus == "completed" || state.missionExecutionStatus == "aborted";
    }

    if (filterKey == QStringLiteral("completed")) {
        return state.missionExecutionStatus == "completed";
    }

    if (filterKey == QStringLiteral("aborted")) {
        return state.missionExecutionStatus == "aborted";
    }

    return true;
}

QString formatMissionExecutionPhase(const std::string& phaseCode)
{
    if (phaseCode == "unassigned") {
        return QStringLiteral("未建立任务");
    }

    if (phaseCode == "task-assigned") {
        return QStringLiteral("任务已下达");
    }

    if (phaseCode == "searching-for-target") {
        return QStringLiteral("搜索目标");
    }

    if (phaseCode == "transit-to-waypoint") {
        return QStringLiteral("前往当前航路点");
    }
    if (phaseCode == "patrol-transit") {
        return QStringLiteral("沿巡逻航路机动");
    }

    if (phaseCode == "intercept-transit") {
        return QStringLiteral("向拦截目标机动");
    }

    if (phaseCode == "orbit-acquire") {
        return QStringLiteral("进入盘旋轨道");
    }

    if (phaseCode == "orbit-station") {
        return QStringLiteral("保持盘旋轨道");
    }

    if (phaseCode == "escort-rejoin") {
        return QStringLiteral("向护航槽位回归");
    }

    if (phaseCode == "escort-station") {
        return QStringLiteral("保持护航编队位");
    }

    if (phaseCode == "holding-at-waypoint") {
        return QStringLiteral("航路点等待");
    }
    if (phaseCode == "patrol-hold") {
        return QStringLiteral("巡逻点等待");
    }

    if (phaseCode == "target-detected") {
        return QStringLiteral("已发现任务目标");
    }
    if (phaseCode == "target-intercept-window") {
        return QStringLiteral("建立拦截窗口");
    }

    if (phaseCode == "intercept-complete") {
        return QStringLiteral("完成拦截任务");
    }

    if (phaseCode == "transit-complete") {
        return QStringLiteral("完成转场任务");
    }

    if (phaseCode == "escort-complete") {
        return QStringLiteral("完成护航任务");
    }

    if (phaseCode == "orbit-complete") {
        return QStringLiteral("完成盘旋建立");
    }

    if (phaseCode == "target-unavailable") {
        return QStringLiteral("任务目标不可用");
    }

    if (phaseCode == "escort-anchor-lost") {
        return QStringLiteral("护航锚点丢失");
    }

    if (phaseCode == "orbit-anchor-lost") {
        return QStringLiteral("盘旋锚点丢失");
    }

    if (phaseCode == "awaiting-escort-anchor") {
        return QStringLiteral("等待护航锚点");
    }

    if (phaseCode == "awaiting-orbit-anchor") {
        return QStringLiteral("等待盘旋中心");
    }

    if (phaseCode == "awaiting-prerequisite") {
        return QStringLiteral("等待前置任务满足");
    }

    if (phaseCode == "timeout-expired") {
        return QStringLiteral("任务执行超时");
    }

    if (phaseCode == "replanned-after-abort") {
        return QStringLiteral("中止后已重规划");
    }

    return QString::fromStdString(phaseCode);
}

QString formatMissionTerminalReason(const std::string& reasonCode)
{
    if (reasonCode.empty()) {
        return QStringLiteral("无");
    }

    if (reasonCode == "target-contained") {
        return QStringLiteral("目标已纳入拦截窗口");
    }

    if (reasonCode == "route-finished") {
        return QStringLiteral("已完成预定航路转场");
    }

    if (reasonCode == "target-unavailable") {
        return QStringLiteral("任务目标不可用");
    }

    if (reasonCode == "formation-stable") {
        return QStringLiteral("护航编队已稳定");
    }

    if (reasonCode == "escort-anchor-lost") {
        return QStringLiteral("护航锚点丢失");
    }

    if (reasonCode == "orbit-established") {
        return QStringLiteral("盘旋轨道已稳定建立");
    }

    if (reasonCode == "orbit-anchor-lost") {
        return QStringLiteral("盘旋锚点丢失");
    }

    if (reasonCode == "timeout-expired") {
        return QStringLiteral("任务执行超时");
    }

    return QString::fromStdString(reasonCode);
}

QString formatMovementGuidanceMode(const std::string& modeCode)
{
    if (modeCode == "inertial") {
        return QStringLiteral("惯性运动");
    }

    if (modeCode == "route") {
        return QStringLiteral("航路引导");
    }

    if (modeCode == "mission-target") {
        return QStringLiteral("任务目标引导");
    }

    if (modeCode == "mission-orbit") {
        return QStringLiteral("任务盘旋引导");
    }

    return QString::fromStdString(modeCode);
}

QString formatMissionBehaviorText(const std::string& behaviorCode)
{
    if (behaviorCode == "patrol") {
        return QStringLiteral("巡逻");
    }

    if (behaviorCode == "intercept") {
        return QStringLiteral("拦截");
    }

    if (behaviorCode == "escort") {
        return QStringLiteral("护航");
    }

    if (behaviorCode == "orbit") {
        return QStringLiteral("盘旋");
    }

    if (behaviorCode == "transit") {
        return QStringLiteral("转场");
    }

    return behaviorCode.empty() ? QStringLiteral("无") : QString::fromStdString(behaviorCode);
}

QString optionalDoubleToEditorText(const std::optional<double>& value)
{
    return value.has_value() ? QString::number(*value, 'f', 2) : QString();
}

QString formatActiveWaypoint(const fm::app::EntityRenderState& state)
{
    if (state.routeWaypoints.empty()) {
        return QStringLiteral("无");
    }

    const QString waypointName = state.activeRouteWaypointName.empty()
                                     ? QStringLiteral("未命名")
                                     : QString::fromStdString(state.activeRouteWaypointName);
    return QStringLiteral("%1 / %2: %3")
        .arg(state.activeRouteWaypointIndex + 1)
        .arg(state.routeWaypoints.size())
        .arg(waypointName);
}

QString buildEntityListText(const fm::app::EntityRenderState& state)
{
    const QString badge = formatMissionStatusBadge(state);
    const QString displayName = badge.isEmpty()
                                    ? QString::fromStdString(state.displayName)
                                    : QStringLiteral("%1 %2").arg(QString::fromStdString(state.displayName), badge);
    return QStringLiteral("%1\n%2")
        .arg(displayName)
        .arg(joinEntityMeta(state));
}

QListWidgetItem* findEntityListItem(QListWidget* entityList, const std::string& entityId)
{
    for (int row = 0; row < entityList->count(); ++row) {
        auto* item = entityList->item(row);
        if (item != nullptr && item->data(Qt::UserRole).toString().toStdString() == entityId) {
            return item;
        }
    }

    return nullptr;
}

void updateEntityListItem(QListWidgetItem* item, const fm::app::EntityRenderState& state)
{
    if (item == nullptr) {
        return;
    }

    item->setText(buildEntityListText(state));
    const QString tooltip = state.missionTerminalReason.empty()
                                ? QStringLiteral("ID: %1").arg(QString::fromStdString(state.id))
                                : QStringLiteral("ID: %1\n终态原因: %2")
                                      .arg(QString::fromStdString(state.id))
                                      .arg(QString::fromStdString(state.missionTerminalReason));
    item->setToolTip(tooltip);
    item->setData(Qt::UserRole, QString::fromStdString(state.id));
    QColor foregroundColor(QString::fromStdString(state.colorHex));
    if (state.missionExecutionStatus == "completed") {
        foregroundColor = QColor("#1F7A4C");
    } else if (state.missionExecutionStatus == "aborted") {
        foregroundColor = QColor("#8A3B2E");
    }

    item->setForeground(foregroundColor);
}

QPointF worldToScene(const fm::core::Vector2& position)
{
    constexpr double scale = 12.0;
    return {position.x * scale, -position.y * scale};
}

QRectF buildSceneRect(const fm::app::SimulationSession& session,
                     const std::vector<fm::app::EntityRenderState>& states)
{
    constexpr double scale = 12.0;
    constexpr double minimumHalfSpanPixels = 320.0;
    constexpr double paddingPixels = 72.0;

    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::lowest();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    if (session.hasScenario()) {
        const auto& bounds = session.currentScenarioMapBounds();
        if (bounds.maximum.x > bounds.minimum.x && bounds.maximum.y > bounds.minimum.y) {
            minX = std::min(minX, bounds.minimum.x * scale);
            maxX = std::max(maxX, bounds.maximum.x * scale);
            minY = std::min(minY, -bounds.maximum.y * scale);
            maxY = std::max(maxY, -bounds.minimum.y * scale);
        }
    }

    for (const auto& state : states) {
        const auto point = worldToScene(state.position);
        minX = std::min(minX, point.x());
        maxX = std::max(maxX, point.x());
        minY = std::min(minY, point.y());
        maxY = std::max(maxY, point.y());

        for (const auto& waypoint : state.routeWaypoints) {
            const auto waypointPoint = worldToScene(waypoint.position);
            minX = std::min(minX, waypointPoint.x());
            maxX = std::max(maxX, waypointPoint.x());
            minY = std::min(minY, waypointPoint.y());
            maxY = std::max(maxY, waypointPoint.y());
        }
    }

    if (minX == std::numeric_limits<double>::max()) {
        return QRectF(-450.0, -320.0, 900.0, 640.0);
    }

    const double centerX = (minX + maxX) * 0.5;
    const double centerY = (minY + maxY) * 0.5;
    const double halfWidth = std::max((maxX - minX) * 0.5 + paddingPixels, minimumHalfSpanPixels);
    const double halfHeight = std::max((maxY - minY) * 0.5 + paddingPixels, minimumHalfSpanPixels * 0.72);
    return QRectF(centerX - halfWidth, centerY - halfHeight, halfWidth * 2.0, halfHeight * 2.0);
}

bool matchesLogFilter(const QString& entry, const QString& filterKey)
{
    const int prefixEnd = entry.indexOf(QStringLiteral("] "));
    const QString normalizedEntry = prefixEnd >= 0 ? entry.mid(prefixEnd + 2) : entry;

    if (filterKey == QStringLiteral("mission")) {
        return normalizedEntry.startsWith(QStringLiteral("任务阶段切换"));
    }

    if (filterKey == QStringLiteral("terminal")) {
        return normalizedEntry.startsWith(QStringLiteral("任务终态"));
    }

    if (filterKey == QStringLiteral("guidance")) {
        return normalizedEntry.startsWith(QStringLiteral("机动引导切换"));
    }

    if (filterKey == QStringLiteral("route")) {
        return normalizedEntry.startsWith(QStringLiteral("航路点到达"));
    }

    if (filterKey == QStringLiteral("system")) {
        return !normalizedEntry.startsWith(QStringLiteral("任务阶段切换"))
            && !normalizedEntry.startsWith(QStringLiteral("任务终态"))
            && !normalizedEntry.startsWith(QStringLiteral("机动引导切换"))
            && !normalizedEntry.startsWith(QStringLiteral("航路点到达"));
    }

    return true;
}

bool matchesSelectedEntityLog(const QString& entry,
                             const std::string& selectedEntityId,
                             const std::vector<fm::app::EntityRenderState>& states)
{
    if (selectedEntityId.empty()) {
        return true;
    }

    QString selectedDisplayName;
    for (const auto& state : states) {
        if (state.id == selectedEntityId) {
            selectedDisplayName = QString::fromStdString(state.displayName.empty() ? state.id : state.displayName);
            break;
        }
    }

    if (selectedDisplayName.isEmpty()) {
        selectedDisplayName = QString::fromStdString(selectedEntityId);
    }

    return entry.contains(selectedDisplayName) || entry.contains(QString::fromStdString(selectedEntityId));
}

}  // namespace

namespace fm::ui {

MainWindow::MainWindow()
    : session_(0.01)
{
    buildUi();
    connectSignals();
    loadDefaultScenario();
    refreshScene();
    updateStatus();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    auto* centralWidget = new QWidget(this);
    auto* rootLayout = new QVBoxLayout(centralWidget);
    auto* buttonLayout = new QHBoxLayout();
    auto* contentLayout = new QHBoxLayout();
    auto* controlLayout = new QHBoxLayout();

    loadButton_ = new QPushButton(QStringLiteral("加载场景"), centralWidget);
    saveButton_ = new QPushButton(QStringLiteral("保存场景"), centralWidget);
    startButton_ = new QPushButton(QStringLiteral("开始"), centralWidget);
    pauseButton_ = new QPushButton(QStringLiteral("暂停"), centralWidget);
    stepButton_ = new QPushButton(QStringLiteral("单步"), centralWidget);
    resetButton_ = new QPushButton(QStringLiteral("重置"), centralWidget);
    previousSnapshotButton_ = new QPushButton(QStringLiteral("上一帧"), centralWidget);
    nextSnapshotButton_ = new QPushButton(QStringLiteral("下一帧"), centralWidget);
    liveViewButton_ = new QPushButton(QStringLiteral("实时视图"), centralWidget);
    zoomInButton_ = new QPushButton(QStringLiteral("放大"), centralWidget);
    zoomOutButton_ = new QPushButton(QStringLiteral("缩小"), centralWidget);
    resetViewButton_ = new QPushButton(QStringLiteral("重置视图"), centralWidget);
    showTrailsCheckBox_ = new QCheckBox(QStringLiteral("显示轨迹"), centralWidget);
    autoFitViewCheckBox_ = new QCheckBox(QStringLiteral("自动适配视图"), centralWidget);
    lockSelectedEntityViewCheckBox_ = new QCheckBox(QStringLiteral("锁定选中实体"), centralWidget);
    selectedEntityLogOnlyCheckBox_ = new QCheckBox(QStringLiteral("仅选中实体日志"), centralWidget);
    entityFilterComboBox_ = new QComboBox(centralWidget);
    logFilterComboBox_ = new QComboBox(centralWidget);
    timeStepSpinBox_ = new QDoubleSpinBox(centralWidget);
    logView_ = new QPlainTextEdit(centralWidget);
    statusLabel_ = new QLabel(centralWidget);
    scenarioLabel_ = new QLabel(centralWidget);
    entityList_ = new QListWidget(centralWidget);
    entityDetailsLabel_ = new QLabel(centralWidget);
    taskEditorHintLabel_ = new QLabel(centralWidget);
    recordingSummaryLabel_ = new QLabel(centralWidget);
    taskObjectiveEdit_ = new QLineEdit(centralWidget);
    taskBehaviorComboBox_ = new QComboBox(centralWidget);
    taskTargetComboBox_ = new QComboBox(centralWidget);
    taskOrbitDirectionComboBox_ = new QComboBox(centralWidget);
    taskInterceptDistanceEdit_ = new QLineEdit(centralWidget);
    taskOrbitRadiusEdit_ = new QLineEdit(centralWidget);
    taskOrbitAcquireToleranceEdit_ = new QLineEdit(centralWidget);
    taskOrbitCompletionToleranceEdit_ = new QLineEdit(centralWidget);
    taskOrbitHoldEdit_ = new QLineEdit(centralWidget);
    taskEscortTrailEdit_ = new QLineEdit(centralWidget);
    taskEscortOffsetEdit_ = new QLineEdit(centralWidget);
    taskEscortSlotToleranceEdit_ = new QLineEdit(centralWidget);
    taskEscortHoldEdit_ = new QLineEdit(centralWidget);
    applyTaskButton_ = new QPushButton(QStringLiteral("应用任务"), centralWidget);

    buttonLayout->addWidget(loadButton_);
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(startButton_);
    buttonLayout->addWidget(pauseButton_);
    buttonLayout->addWidget(stepButton_);
    buttonLayout->addWidget(resetButton_);
    buttonLayout->addWidget(previousSnapshotButton_);
    buttonLayout->addWidget(nextSnapshotButton_);
    buttonLayout->addWidget(liveViewButton_);
    buttonLayout->addWidget(zoomInButton_);
    buttonLayout->addWidget(zoomOutButton_);
    buttonLayout->addWidget(resetViewButton_);
    buttonLayout->addStretch();

    showTrailsCheckBox_->setChecked(true);
    autoFitViewCheckBox_->setChecked(true);
    entityFilterComboBox_->addItem(QStringLiteral("全部实体"), QStringLiteral("all"));
    entityFilterComboBox_->addItem(QStringLiteral("活动任务"), QStringLiteral("active"));
    entityFilterComboBox_->addItem(QStringLiteral("终态任务"), QStringLiteral("terminal"));
    entityFilterComboBox_->addItem(QStringLiteral("已完成"), QStringLiteral("completed"));
    entityFilterComboBox_->addItem(QStringLiteral("已中止"), QStringLiteral("aborted"));
    logFilterComboBox_->addItem(QStringLiteral("全部日志"), QStringLiteral("all"));
    logFilterComboBox_->addItem(QStringLiteral("任务阶段"), QStringLiteral("mission"));
    logFilterComboBox_->addItem(QStringLiteral("任务终态"), QStringLiteral("terminal"));
    logFilterComboBox_->addItem(QStringLiteral("机动引导"), QStringLiteral("guidance"));
    logFilterComboBox_->addItem(QStringLiteral("航路到达"), QStringLiteral("route"));
    logFilterComboBox_->addItem(QStringLiteral("系统事件"), QStringLiteral("system"));
    taskBehaviorComboBox_->addItem(QStringLiteral("巡逻"), QStringLiteral("patrol"));
    taskBehaviorComboBox_->addItem(QStringLiteral("拦截"), QStringLiteral("intercept"));
    taskBehaviorComboBox_->addItem(QStringLiteral("护航"), QStringLiteral("escort"));
    taskBehaviorComboBox_->addItem(QStringLiteral("盘旋"), QStringLiteral("orbit"));
    taskBehaviorComboBox_->addItem(QStringLiteral("转场"), QStringLiteral("transit"));
    taskTargetComboBox_->addItem(QStringLiteral("无"), QString());
    taskOrbitDirectionComboBox_->addItem(QStringLiteral("默认"), QString());
    taskOrbitDirectionComboBox_->addItem(QStringLiteral("顺时针"), QStringLiteral("clockwise"));
    taskOrbitDirectionComboBox_->addItem(QStringLiteral("逆时针"), QStringLiteral("counterclockwise"));
    timeStepSpinBox_->setDecimals(3);
    timeStepSpinBox_->setRange(0.01, 1.0);
    timeStepSpinBox_->setSingleStep(0.01);
    timeStepSpinBox_->setSuffix(QStringLiteral(" s/tick"));
    timeStepSpinBox_->setValue(session_.fixedTimeStepSeconds());

    controlLayout->addWidget(showTrailsCheckBox_);
    controlLayout->addWidget(autoFitViewCheckBox_);
    controlLayout->addWidget(lockSelectedEntityViewCheckBox_);
    controlLayout->addWidget(new QLabel(QStringLiteral("实体过滤"), centralWidget));
    controlLayout->addWidget(entityFilterComboBox_);
    controlLayout->addWidget(new QLabel(QStringLiteral("固定时间步"), centralWidget));
    controlLayout->addWidget(timeStepSpinBox_);
    controlLayout->addStretch();

    scene_ = new QGraphicsScene(this);
    scene_->setSceneRect(-450.0, -320.0, 900.0, 640.0);
    auto* zoomableView = new ZoomableGraphicsView(scene_, centralWidget);
    zoomableView->setZoomCallback([this]() {
        if (autoFitViewCheckBox_ != nullptr) {
            autoFitViewCheckBox_->setChecked(false);
        }
    });
    view_ = zoomableView;
    view_->setRenderHint(QPainter::Antialiasing, true);
    view_->setMinimumSize(900, 640);
    view_->setDragMode(QGraphicsView::ScrollHandDrag);
    view_->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    view_->setResizeAnchor(QGraphicsView::AnchorViewCenter);

    entityList_->setMinimumWidth(260);
    entityList_->setMinimumHeight(80);
    entityDetailsLabel_->setWordWrap(true);
    entityDetailsLabel_->setMinimumWidth(220);
    entityDetailsLabel_->setMinimumHeight(0);
    entityDetailsLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    entityDetailsLabel_->setStyleSheet(QStringLiteral("QLabel { background: #F3F5F1; border: 1px solid #C5CEC7; padding: 10px; }"));
    taskEditorHintLabel_->setWordWrap(true);
    taskEditorHintLabel_->setText(QStringLiteral("选择实体后可修改任务；数值留空表示使用默认参数。应用后会重置仿真到初始态。"));
    taskObjectiveEdit_->setPlaceholderText(QStringLiteral("任务说明"));
    taskInterceptDistanceEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskOrbitRadiusEdit_->setPlaceholderText(QStringLiteral("默认/自动"));
    taskOrbitAcquireToleranceEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskOrbitCompletionToleranceEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskOrbitHoldEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskEscortTrailEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskEscortOffsetEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskEscortSlotToleranceEdit_->setPlaceholderText(QStringLiteral("默认"));
    taskEscortHoldEdit_->setPlaceholderText(QStringLiteral("默认"));
    recordingSummaryLabel_->setWordWrap(true);
    recordingSummaryLabel_->setMinimumWidth(220);
    recordingSummaryLabel_->setMinimumHeight(0);
    recordingSummaryLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    recordingSummaryLabel_->setStyleSheet(QStringLiteral("QLabel { background: #F8FAF7; border: 1px solid #C5CEC7; padding: 10px; }"));
    logView_->setReadOnly(true);
    logView_->setMinimumHeight(80);
    logView_->setStyleSheet(QStringLiteral("QPlainTextEdit { background: #F8FAF7; border: 1px solid #C5CEC7; padding: 6px; }"));

    auto* entityDetailsScrollArea = new QScrollArea(centralWidget);
    entityDetailsScrollArea->setWidgetResizable(true);
    entityDetailsScrollArea->setFrameShape(QFrame::NoFrame);
    entityDetailsScrollArea->setWidget(entityDetailsLabel_);
    entityDetailsScrollArea->setMinimumHeight(80);

    auto* recordingSummaryScrollArea = new QScrollArea(centralWidget);
    recordingSummaryScrollArea->setWidgetResizable(true);
    recordingSummaryScrollArea->setFrameShape(QFrame::NoFrame);
    recordingSummaryScrollArea->setWidget(recordingSummaryLabel_);
    recordingSummaryScrollArea->setMinimumHeight(60);

    auto* entityListPanel = new QWidget(centralWidget);
    auto* entityListLayout = new QVBoxLayout(entityListPanel);
    entityListLayout->setContentsMargins(0, 0, 0, 0);
    entityListLayout->addWidget(new QLabel(QStringLiteral("实体列表"), entityListPanel));
    entityListLayout->addWidget(entityList_);

    auto* entityDetailsPanel = new QWidget(centralWidget);
    auto* entityDetailsLayout = new QVBoxLayout(entityDetailsPanel);
    entityDetailsLayout->setContentsMargins(0, 0, 0, 0);
    entityDetailsLayout->addWidget(new QLabel(QStringLiteral("实体详情"), entityDetailsPanel));
    entityDetailsLayout->addWidget(entityDetailsScrollArea);

    auto* taskEditorForm = new QFormLayout();
    taskEditorForm->addRow(QStringLiteral("任务说明"), taskObjectiveEdit_);
    taskEditorForm->addRow(QStringLiteral("任务行为"), taskBehaviorComboBox_);
    taskEditorForm->addRow(QStringLiteral("任务目标"), taskTargetComboBox_);
    taskEditorForm->addRow(QStringLiteral("拦截完成距离"), taskInterceptDistanceEdit_);
    taskEditorForm->addRow(QStringLiteral("盘旋半径"), taskOrbitRadiusEdit_);
    taskEditorForm->addRow(QStringLiteral("盘旋方向"), taskOrbitDirectionComboBox_);
    taskEditorForm->addRow(QStringLiteral("盘旋进入容差"), taskOrbitAcquireToleranceEdit_);
    taskEditorForm->addRow(QStringLiteral("盘旋完成容差"), taskOrbitCompletionToleranceEdit_);
    taskEditorForm->addRow(QStringLiteral("盘旋保持时间"), taskOrbitHoldEdit_);
    taskEditorForm->addRow(QStringLiteral("护航尾随距离"), taskEscortTrailEdit_);
    taskEditorForm->addRow(QStringLiteral("护航侧向偏移"), taskEscortOffsetEdit_);
    taskEditorForm->addRow(QStringLiteral("护航槽位容差"), taskEscortSlotToleranceEdit_);
    taskEditorForm->addRow(QStringLiteral("护航稳定保持"), taskEscortHoldEdit_);

    auto* taskEditorContent = new QWidget(centralWidget);
    auto* taskEditorContentLayout = new QVBoxLayout(taskEditorContent);
    taskEditorContentLayout->setContentsMargins(0, 0, 0, 0);
    taskEditorContentLayout->addWidget(taskEditorHintLabel_);
    taskEditorContentLayout->addLayout(taskEditorForm);
    taskEditorContentLayout->addWidget(applyTaskButton_);

    auto* taskEditorScrollArea = new QScrollArea(centralWidget);
    taskEditorScrollArea->setWidgetResizable(true);
    taskEditorScrollArea->setFrameShape(QFrame::NoFrame);
    taskEditorScrollArea->setWidget(taskEditorContent);
    taskEditorScrollArea->setMinimumHeight(160);

    auto* taskEditorPanel = new QWidget(centralWidget);
    auto* taskEditorLayout = new QVBoxLayout(taskEditorPanel);
    taskEditorLayout->setContentsMargins(0, 0, 0, 0);
    taskEditorLayout->addWidget(new QLabel(QStringLiteral("任务编辑"), taskEditorPanel));
    taskEditorLayout->addWidget(taskEditorScrollArea);

    auto* recordingPanel = new QWidget(centralWidget);
    auto* recordingLayout = new QVBoxLayout(recordingPanel);
    recordingLayout->setContentsMargins(0, 0, 0, 0);
    recordingLayout->addWidget(new QLabel(QStringLiteral("记录摘要"), recordingPanel));
    recordingLayout->addWidget(recordingSummaryScrollArea);

    auto* logPanel = new QWidget(centralWidget);
    auto* logLayout = new QVBoxLayout(logPanel);
    logLayout->setContentsMargins(0, 0, 0, 0);
    logLayout->addWidget(new QLabel(QStringLiteral("事件日志"), logPanel));
    logLayout->addWidget(logFilterComboBox_);
    logLayout->addWidget(selectedEntityLogOnlyCheckBox_);
    logLayout->addWidget(logView_);

    auto* sideSplitter = new QSplitter(Qt::Vertical, centralWidget);
    sideSplitter->addWidget(entityListPanel);
    sideSplitter->addWidget(entityDetailsPanel);
    sideSplitter->addWidget(taskEditorPanel);
    sideSplitter->addWidget(recordingPanel);
    sideSplitter->addWidget(logPanel);
    sideSplitter->setChildrenCollapsible(false);
    sideSplitter->setHandleWidth(8);
    sideSplitter->setStretchFactor(0, 2);
    sideSplitter->setStretchFactor(1, 4);
    sideSplitter->setStretchFactor(2, 4);
    sideSplitter->setStretchFactor(3, 2);
    sideSplitter->setStretchFactor(4, 3);
    sideSplitter->setSizes({180, 260, 300, 150, 220});
    sideSplitter->setMinimumWidth(300);

    contentLayout->addWidget(view_, 1);
    contentLayout->addWidget(sideSplitter);

    rootLayout->addLayout(buttonLayout);
    rootLayout->addLayout(controlLayout);
    rootLayout->addWidget(scenarioLabel_);
    rootLayout->addWidget(statusLabel_);
    rootLayout->addLayout(contentLayout);

    setCentralWidget(centralWidget);
    setWindowTitle(QStringLiteral("FunctionalModelTest - MVP 骨架"));

    timer_ = new QTimer(this);
    timer_->setInterval(50);

    updateTaskEditorFieldState();
}

void MainWindow::connectSignals()
{
    connect(loadButton_, &QPushButton::clicked, this, [this]() {
        const auto defaultDir = QApplication::applicationDirPath() + QStringLiteral("/assets/scenarios");
        const auto filePath = QFileDialog::getOpenFileName(
            this,
            QStringLiteral("选择场景文件"),
            defaultDir,
            QStringLiteral("JSON Files (*.json)"));

        if (filePath.isEmpty()) {
            return;
        }

        if (!session_.loadScenario(filePath)) {
            QMessageBox::critical(this, QStringLiteral("加载失败"), session_.lastError());
        }

        forceFitViewOnNextRefresh_ = true;
        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(saveButton_, &QPushButton::clicked, this, [this]() {
        const QString suggestedPath = !session_.currentScenarioPath().isEmpty()
                                          ? session_.currentScenarioPath()
                                          : (QApplication::applicationDirPath() + QStringLiteral("/assets/scenarios/edited_scenario.json"));
        const auto filePath = QFileDialog::getSaveFileName(
            this,
            QStringLiteral("保存场景文件"),
            suggestedPath,
            QStringLiteral("JSON Files (*.json)"));

        if (filePath.isEmpty()) {
            return;
        }

        if (!session_.saveScenarioToFile(filePath)) {
            QMessageBox::warning(this, QStringLiteral("保存失败"), session_.lastError());
            return;
        }

        updateStatus();
        QMessageBox::information(this, QStringLiteral("保存成功"), QStringLiteral("场景已保存到:\n%1").arg(filePath));
    });

    connect(startButton_, &QPushButton::clicked, this, [this]() {
        replaySnapshotIndex_ = -1;
        session_.start();
        timer_->start();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(pauseButton_, &QPushButton::clicked, this, [this]() {
        session_.pause();
        timer_->stop();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(stepButton_, &QPushButton::clicked, this, [this]() {
        replaySnapshotIndex_ = -1;
        session_.step();
        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(resetButton_, &QPushButton::clicked, this, [this]() {
        timer_->stop();
        replaySnapshotIndex_ = -1;
        session_.reset();
        forceFitViewOnNextRefresh_ = true;
        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(timer_, &QTimer::timeout, this, [this]() {
        replaySnapshotIndex_ = -1;
        session_.tick();
        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateStatus();
    });

    connect(previousSnapshotButton_, &QPushButton::clicked, this, [this]() {
        if (session_.recording().empty()) {
            return;
        }

        timer_->stop();
        session_.pause();
        if (replaySnapshotIndex_ < 0) {
            replaySnapshotIndex_ = static_cast<int>(session_.recording().size()) - 1;
        } else if (replaySnapshotIndex_ > 0) {
            --replaySnapshotIndex_;
        }

        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(nextSnapshotButton_, &QPushButton::clicked, this, [this]() {
        if (session_.recording().empty()) {
            return;
        }

        timer_->stop();
        session_.pause();
        if (replaySnapshotIndex_ < 0) {
            replaySnapshotIndex_ = static_cast<int>(session_.recording().size()) - 1;
        } else if (replaySnapshotIndex_ + 1 < static_cast<int>(session_.recording().size())) {
            ++replaySnapshotIndex_;
        }

        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateLogView();
        updateStatus();
    });

    connect(liveViewButton_, &QPushButton::clicked, this, [this]() {
        replaySnapshotIndex_ = -1;
        refreshScene();
        updateEntityList();
        updateEntityDetails();
        updateRecordingSummary();
        updateStatus();
    });

    connect(showTrailsCheckBox_, &QCheckBox::toggled, this, [this]() {
        refreshScene();
    });

    connect(autoFitViewCheckBox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked) {
            refreshScene();
        }
    });

    connect(lockSelectedEntityViewCheckBox_, &QCheckBox::toggled, this, [this](bool) {
        refreshScene();
    });

    connect(entityFilterComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        updateEntityList();
        updateEntityDetails();
    });

    connect(logFilterComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        updateLogView();
    });

    connect(selectedEntityLogOnlyCheckBox_, &QCheckBox::toggled, this, [this](bool) {
        updateLogView();
    });

    connect(taskBehaviorComboBox_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int) {
        updateTaskEditorFieldState();
    });

    connect(applyTaskButton_, &QPushButton::clicked, this, [this]() {
        applyTaskEditorChanges();
    });

    connect(zoomInButton_, &QPushButton::clicked, this, [this]() {
        if (autoFitViewCheckBox_ != nullptr) {
            autoFitViewCheckBox_->setChecked(false);
        }
        view_->scale(1.2, 1.2);
    });

    connect(zoomOutButton_, &QPushButton::clicked, this, [this]() {
        if (autoFitViewCheckBox_ != nullptr) {
            autoFitViewCheckBox_->setChecked(false);
        }
        view_->scale(1.0 / 1.2, 1.0 / 1.2);
    });

    connect(resetViewButton_, &QPushButton::clicked, this, [this]() {
        view_->resetTransform();
        refreshScene();
    });

    connect(timeStepSpinBox_, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this](double value) {
        session_.setFixedTimeStepSeconds(value);
        updateLogView();
        updateStatus();
    });

    connect(entityList_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        selectedEntityId_ = current != nullptr ? current->data(Qt::UserRole).toString().toStdString() : std::string {};
        updateTaskEditor();
        updateEntityDetails();
        refreshScene();
    });
}

void MainWindow::loadDefaultScenario()
{
    const auto defaultScenario = QApplication::applicationDirPath() + QStringLiteral("/assets/scenarios/sample_scenario.json");
    session_.loadScenario(defaultScenario);
    forceFitViewOnNextRefresh_ = true;
    updateEntityList();
    updateTaskEditor();
    updateEntityDetails();
    updateRecordingSummary();
    updateLogView();
}

const fm::app::SimulationSnapshot* MainWindow::currentSnapshot() const
{
    if (isReplayMode()) {
        return session_.snapshotAt(static_cast<std::size_t>(replaySnapshotIndex_));
    }

    return session_.latestSnapshot();
}

std::vector<fm::app::EntityRenderState> MainWindow::currentRenderStates() const
{
    if (isReplayMode()) {
        return session_.renderStatesAt(static_cast<std::size_t>(replaySnapshotIndex_));
    }

    return session_.renderStates();
}

std::vector<fm::app::EntityRenderState> MainWindow::filteredEntityListStates() const
{
    auto states = currentRenderStates();
    const QString filterKey = entityFilterComboBox_ != nullptr
                                  ? entityFilterComboBox_->currentData().toString()
                                  : QStringLiteral("all");

    states.erase(std::remove_if(states.begin(),
                                states.end(),
                                [&filterKey](const fm::app::EntityRenderState& state) {
                                    return !matchesEntityFilter(state, filterKey);
                                }),
                 states.end());

    std::stable_sort(states.begin(), states.end(), [](const fm::app::EntityRenderState& lhs, const fm::app::EntityRenderState& rhs) {
        const int lhsPriority = missionStatusSortPriority(lhs);
        const int rhsPriority = missionStatusSortPriority(rhs);
        if (lhsPriority != rhsPriority) {
            return lhsPriority < rhsPriority;
        }

        return lhs.displayName < rhs.displayName;
    });

    return states;
}

bool MainWindow::isReplayMode() const
{
    return replaySnapshotIndex_ >= 0;
}

void MainWindow::refreshScene()
{
    const auto states = currentRenderStates();
    const QRectF sceneRect = buildSceneRect(session_, states);
    scene_->clear();
    scene_->setSceneRect(sceneRect);

    const QPen axisPen(QColor("#73877B"), 1.0, Qt::DashLine);
    const QPen gridPen(QColor("#D8E0D5"), 1.0);
    const double gridStep = 60.0;
    const double left = std::floor(sceneRect.left() / gridStep) * gridStep;
    const double right = std::ceil(sceneRect.right() / gridStep) * gridStep;
    const double top = std::floor(sceneRect.top() / gridStep) * gridStep;
    const double bottom = std::ceil(sceneRect.bottom() / gridStep) * gridStep;

    for (double x = left; x <= right; x += gridStep) {
        scene_->addLine(x, sceneRect.top(), x, sceneRect.bottom(), gridPen);
    }

    for (double y = top; y <= bottom; y += gridStep) {
        scene_->addLine(sceneRect.left(), y, sceneRect.right(), y, gridPen);
    }

    if (sceneRect.left() <= 0.0 && sceneRect.right() >= 0.0) {
        scene_->addLine(0.0, sceneRect.top(), 0.0, sceneRect.bottom(), axisPen);
    }

    if (sceneRect.top() <= 0.0 && sceneRect.bottom() >= 0.0) {
        scene_->addLine(sceneRect.left(), 0.0, sceneRect.right(), 0.0, axisPen);
    }

    if (session_.hasScenario()) {
        const auto& bounds = session_.currentScenarioMapBounds();
        if (bounds.maximum.x > bounds.minimum.x && bounds.maximum.y > bounds.minimum.y) {
            const QPointF topLeft = worldToScene({bounds.minimum.x, bounds.maximum.y});
            const QPointF bottomRight = worldToScene({bounds.maximum.x, bounds.minimum.y});
            scene_->addRect(QRectF(topLeft, bottomRight).normalized(), QPen(QColor("#8AA399"), 2.0, Qt::DashLine));
        }
    }
    std::unordered_set<std::string> detectedBySelected;
    const fm::app::EntityRenderState* selectedState = nullptr;
    QPointF selectedCenter;
    bool hasSelectedCenter = false;

    for (const auto& state : states) {
        if (state.id == selectedEntityId_) {
            selectedState = &state;
            selectedCenter = worldToScene(state.position);
            hasSelectedCenter = true;
            detectedBySelected.insert(state.detectedTargetIds.begin(), state.detectedTargetIds.end());
            break;
        }
    }

    for (const auto& state : states) {
        const auto center = worldToScene(state.position);
        const auto trajectory = session_.trajectoryForEntity(state.id);
        QColor color(QString::fromStdString(state.colorHex));
        if (!color.isValid()) {
            color = QColor("#4C956C");
        }

        const bool isSelected = state.id == selectedEntityId_;
        const bool isDetectedBySelected = detectedBySelected.find(state.id) != detectedBySelected.end();
        bool isInSelectedSensorRange = false;
        bool isMissedBySelectedSensor = false;
        if (!isSelected && selectedState != nullptr && selectedState->sensorRangeMeters > 0.0) {
            const double dx = state.position.x - selectedState->position.x;
            const double dy = state.position.y - selectedState->position.y;
            const double distanceSquared = dx * dx + dy * dy;
            const double sensorRangeSquared = selectedState->sensorRangeMeters * selectedState->sensorRangeMeters;
            const bool isKnownFriendly = !selectedState->side.empty() && !state.side.empty() && selectedState->side == state.side;
            isInSelectedSensorRange = !isKnownFriendly && distanceSquared <= sensorRangeSquared;
            isMissedBySelectedSensor = isInSelectedSensorRange && !isDetectedBySelected
                && !pointFallsWithinSensorField(*selectedState, state.position);
        }

        if (isSelected && !state.routeWaypoints.empty()) {
            QPen routePen(color.lighter(115), 1.8, Qt::DashDotLine);
            for (std::size_t waypointIndex = 0; waypointIndex < state.routeWaypoints.size(); ++waypointIndex) {
                const auto& waypoint = state.routeWaypoints[waypointIndex];
                const auto waypointPoint = worldToScene(waypoint.position);
                if (waypointIndex > 0) {
                    const auto previousWaypointPoint = worldToScene(state.routeWaypoints[waypointIndex - 1].position);
                    scene_->addLine(previousWaypointPoint.x(), previousWaypointPoint.y(), waypointPoint.x(), waypointPoint.y(), routePen);
                }
                scene_->addRect(waypointPoint.x() - 4.0, waypointPoint.y() - 4.0, 8.0, 8.0,
                                QPen(color.darker(130), 1.4), QBrush(color.lighter(150)));

                auto* waypointLabel = scene_->addSimpleText(
                    QStringLiteral("%1.%2").arg(waypointIndex + 1).arg(QString::fromStdString(waypoint.name)));
                waypointLabel->setBrush(QColor("#32433A"));
                waypointLabel->setPos(waypointPoint.x() + 8.0, waypointPoint.y() - 10.0);
                if (waypointIndex == state.activeRouteWaypointIndex) {
                    scene_->addEllipse(waypointPoint.x() - 7.0, waypointPoint.y() - 7.0, 14.0, 14.0,
                                       QPen(color.darker(150), 1.5, Qt::DashLine));
                }
            }
        }

        if (isSelected && state.sensorRangeMeters > 0.0) {
            constexpr double scale = 12.0;
            const auto radius = state.sensorRangeMeters * scale;
            if (state.sensorFieldOfViewDegrees >= 359.999) {
                scene_->addEllipse(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0,
                                   QPen(color.lighter(120), 1.2, Qt::DashLine));
            } else {
                const double halfFieldOfViewRadians = state.sensorFieldOfViewDegrees * 0.5 * kPi / 180.0;
                const double facingRadians = sensorFacingDegrees(state) * kPi / 180.0;
                const double startRadians = facingRadians + halfFieldOfViewRadians;
                const double endRadians = facingRadians - halfFieldOfViewRadians;
                const int segmentCount = std::max(12, static_cast<int>(std::ceil(state.sensorFieldOfViewDegrees / 8.0)));

                QPainterPath sensorSweep(center);
                sensorSweep.lineTo(center.x() + std::cos(startRadians) * radius,
                                   center.y() - std::sin(startRadians) * radius);
                for (int segmentIndex = 1; segmentIndex <= segmentCount; ++segmentIndex) {
                    const double interpolation = static_cast<double>(segmentIndex) / static_cast<double>(segmentCount);
                    const double angle = startRadians + (endRadians - startRadians) * interpolation;
                    sensorSweep.lineTo(center.x() + std::cos(angle) * radius,
                                       center.y() - std::sin(angle) * radius);
                }
                sensorSweep.closeSubpath();
                scene_->addPath(sensorSweep,
                                QPen(color.lighter(120), 1.2, Qt::DashLine),
                                QBrush(color.lighter(160), Qt::Dense6Pattern));
            }
        }

        if (!isSelected && isDetectedBySelected) {
            scene_->addEllipse(center.x() - 13.0, center.y() - 13.0, 26.0, 26.0,
                               QPen(QColor("#D64545"), 2.0, Qt::DashLine));
            if (hasSelectedCenter) {
                scene_->addLine(selectedCenter.x(), selectedCenter.y(), center.x(), center.y(),
                                QPen(QColor("#D64545"), 1.6, Qt::DotLine));
            }
        } else if (!isSelected && isMissedBySelectedSensor) {
            scene_->addEllipse(center.x() - 12.0, center.y() - 12.0, 24.0, 24.0,
                               QPen(QColor("#D98E04"), 1.8, Qt::DashDotLine));
        }

        if (showTrailsCheckBox_->isChecked() && trajectory.size() >= 2) {
            QPen trailPen(color.lighter(135), state.id == selectedEntityId_ ? 2.8 : 1.6);
            for (std::size_t index = 1; index < trajectory.size(); ++index) {
                const auto from = worldToScene(trajectory[index - 1]);
                const auto to = worldToScene(trajectory[index]);
                scene_->addLine(from.x(), from.y(), to.x(), to.y(), trailPen);
            }
        }

        QColor entityOutlineColor = isSelected
            ? QColor("#102A43")
            : (isDetectedBySelected ? QColor("#D64545") : (isMissedBySelectedSensor ? QColor("#D98E04") : color.darker(130)));
        if (state.missionExecutionStatus == "completed") {
            entityOutlineColor = QColor("#1F7A4C");
        } else if (state.missionExecutionStatus == "aborted") {
            entityOutlineColor = QColor("#8A3B2E");
        }

        const QPen entityPen(entityOutlineColor, isSelected ? 3.0 : (isDetectedBySelected ? 2.6 : 2.0));
        const QBrush entityBrush(color);
        scene_->addEllipse(center.x() - 8.0, center.y() - 8.0, 16.0, 16.0, entityPen, entityBrush);

        if (state.missionExecutionStatus == "completed") {
            scene_->addLine(center.x() - 4.0, center.y() + 0.5, center.x() - 0.8, center.y() + 4.0,
                            QPen(QColor("#1F7A4C"), 2.2));
            scene_->addLine(center.x() - 0.8, center.y() + 4.0, center.x() + 5.2, center.y() - 4.5,
                            QPen(QColor("#1F7A4C"), 2.2));
        } else if (state.missionExecutionStatus == "aborted") {
            scene_->addLine(center.x() - 4.5, center.y() - 4.5, center.x() + 4.5, center.y() + 4.5,
                            QPen(QColor("#8A3B2E"), 2.0));
            scene_->addLine(center.x() - 4.5, center.y() + 4.5, center.x() + 4.5, center.y() - 4.5,
                            QPen(QColor("#8A3B2E"), 2.0));
        }

        constexpr double minimumRenderableSpeed = 1e-3;
        const double speed = std::hypot(state.velocity.x, state.velocity.y);
        QPointF tip;
        if (speed > minimumRenderableSpeed) {
            const double normalizedX = state.velocity.x / speed;
            const double normalizedY = state.velocity.y / speed;
            tip = QPointF(center.x() + normalizedX * 18.0, center.y() - normalizedY * 18.0);
        } else {
            const auto radians = state.headingDegrees * kPi / 180.0;
            tip = QPointF(center.x() + std::cos(radians) * 18.0, center.y() - std::sin(radians) * 18.0);
        }
        scene_->addLine(center.x(), center.y(), tip.x(), tip.y(),
                QPen(isSelected
                     ? QColor("#102A43")
                     : (isDetectedBySelected ? QColor("#D64545")
                                 : (isMissedBySelectedSensor ? QColor("#D98E04") : color.darker(150))),
                     2.0));

        const QString displayLabel = formatMissionStatusBadge(state).isEmpty()
                                         ? QString::fromStdString(state.displayName)
                                         : QStringLiteral("%1 %2")
                                               .arg(QString::fromStdString(state.displayName), formatMissionStatusBadge(state));
        auto* label = scene_->addSimpleText(displayLabel);
        QColor labelColor = isSelected
            ? QColor("#102A43")
            : (isDetectedBySelected ? QColor("#B83232") : (isMissedBySelectedSensor ? QColor("#A06100") : QColor("#1F2D2A")));
        if (state.missionExecutionStatus == "completed") {
            labelColor = QColor("#1F7A4C");
        } else if (state.missionExecutionStatus == "aborted") {
            labelColor = QColor("#8A3B2E");
        }
        label->setBrush(labelColor);
        label->setPos(center.x() + 10.0, center.y() - 24.0);
    }

    const bool canApplyViewFit = view_ != nullptr
        && view_->isVisible()
        && view_->viewport() != nullptr
        && !view_->viewport()->size().isEmpty();

    if (canApplyViewFit && (forceFitViewOnNextRefresh_ || (autoFitViewCheckBox_ != nullptr && autoFitViewCheckBox_->isChecked()))) {
        view_->fitInView(sceneRect, Qt::KeepAspectRatio);
    } else if (lockSelectedEntityViewCheckBox_ != nullptr && lockSelectedEntityViewCheckBox_->isChecked() && hasSelectedCenter) {
        view_->centerOn(selectedCenter);
    }

    if (canApplyViewFit) {
        forceFitViewOnNextRefresh_ = false;
    }
}

void MainWindow::updateEntityList()
{
    const auto states = filteredEntityListStates();
    const QSignalBlocker blocker(entityList_);

    int targetRow = 0;
    for (const auto& state : states) {
        auto* rowItem = targetRow < entityList_->count() ? entityList_->item(targetRow) : nullptr;
        const auto targetId = QString::fromStdString(state.id);

        if (rowItem != nullptr && rowItem->data(Qt::UserRole).toString() == targetId) {
            updateEntityListItem(rowItem, state);
        } else {
            auto* existingItem = findEntityListItem(entityList_, state.id);
            if (existingItem != nullptr) {
                const auto existingRow = entityList_->row(existingItem);
                auto* movedItem = entityList_->takeItem(existingRow);
                entityList_->insertItem(targetRow, movedItem);
                updateEntityListItem(movedItem, state);
            } else {
                auto* newItem = new QListWidgetItem();
                entityList_->insertItem(targetRow, newItem);
                updateEntityListItem(newItem, state);
            }
        }

        ++targetRow;
    }

    while (entityList_->count() > static_cast<int>(states.size())) {
        delete entityList_->takeItem(entityList_->count() - 1);
    }

    bool selectedFound = false;
    for (const auto& state : states) {
        if (!selectedEntityId_.empty() && state.id == selectedEntityId_) {
            auto* selectedItem = findEntityListItem(entityList_, state.id);
            if (selectedItem != nullptr) {
                entityList_->setCurrentItem(selectedItem);
            }
            selectedFound = true;
        }
    }

    if (!selectedFound) {
        if (!states.empty()) {
            selectedEntityId_ = states.front().id;
            entityList_->setCurrentRow(0);
        } else {
            selectedEntityId_.clear();
        }
    }

    updateTaskEditor();
}

void MainWindow::updateTaskEditor()
{
    const auto* scenario = session_.scenarioDefinition();
    if (scenario == nullptr || selectedEntityId_.empty()) {
        taskObjectiveEdit_->clear();
        taskTargetComboBox_->clear();
        taskTargetComboBox_->addItem(QStringLiteral("无"), QString());
        taskBehaviorComboBox_->setCurrentIndex(0);
        taskOrbitDirectionComboBox_->setCurrentIndex(0);
        taskInterceptDistanceEdit_->clear();
        taskOrbitRadiusEdit_->clear();
        taskOrbitAcquireToleranceEdit_->clear();
        taskOrbitCompletionToleranceEdit_->clear();
        taskOrbitHoldEdit_->clear();
        taskEscortTrailEdit_->clear();
        taskEscortOffsetEdit_->clear();
        taskEscortSlotToleranceEdit_->clear();
        taskEscortHoldEdit_->clear();
        taskEditorHintLabel_->setText(QStringLiteral("选择实体后可修改任务；数值留空表示使用默认参数。应用后会重置仿真到初始态。"));
        applyTaskButton_->setEnabled(false);
        updateTaskEditorFieldState();
        return;
    }

    const fm::app::EntityDefinition* selectedDefinition = nullptr;
    for (const auto& entity : scenario->entities) {
        if (entity.identity.id == selectedEntityId_) {
            selectedDefinition = &entity;
            break;
        }
    }

    if (selectedDefinition == nullptr) {
        applyTaskButton_->setEnabled(false);
        return;
    }

    const QSignalBlocker behaviorBlocker(taskBehaviorComboBox_);
    const QSignalBlocker targetBlocker(taskTargetComboBox_);
    const QSignalBlocker orbitDirectionBlocker(taskOrbitDirectionComboBox_);

    taskObjectiveEdit_->setText(QString::fromStdString(selectedDefinition->mission.objective));

    const int behaviorIndex = taskBehaviorComboBox_->findData(QString::fromStdString(selectedDefinition->mission.behavior));
    taskBehaviorComboBox_->setCurrentIndex(behaviorIndex >= 0 ? behaviorIndex : 0);

    taskTargetComboBox_->clear();
    taskTargetComboBox_->addItem(QStringLiteral("无"), QString());
    for (const auto& entity : scenario->entities) {
        const QString displayName = entity.identity.displayName.empty()
                                        ? QString::fromStdString(entity.identity.id)
                                        : QStringLiteral("%1 (%2)").arg(QString::fromStdString(entity.identity.displayName),
                                                                           QString::fromStdString(entity.identity.id));
        taskTargetComboBox_->addItem(displayName, QString::fromStdString(entity.identity.id));
    }
    const int targetIndex = taskTargetComboBox_->findData(QString::fromStdString(selectedDefinition->mission.targetEntityId));
    taskTargetComboBox_->setCurrentIndex(targetIndex >= 0 ? targetIndex : 0);

    taskInterceptDistanceEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.interceptCompletionDistanceMeters));
    taskOrbitRadiusEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.orbitRadiusMeters));
    taskOrbitAcquireToleranceEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.orbitAcquireToleranceMeters));
    taskOrbitCompletionToleranceEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.orbitCompletionToleranceMeters));
    taskOrbitHoldEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.orbitCompletionHoldSeconds));
    taskEscortTrailEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.escortTrailDistanceMeters));
    taskEscortOffsetEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.escortRightOffsetMeters));
    taskEscortSlotToleranceEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.escortSlotToleranceMeters));
    taskEscortHoldEdit_->setText(optionalDoubleToEditorText(selectedDefinition->mission.parameters.escortCompletionHoldSeconds));

    if (!selectedDefinition->mission.parameters.orbitClockwise.has_value()) {
        taskOrbitDirectionComboBox_->setCurrentIndex(0);
    } else {
        taskOrbitDirectionComboBox_->setCurrentIndex(*selectedDefinition->mission.parameters.orbitClockwise ? 1 : 2);
    }

    taskEditorHintLabel_->setText(QStringLiteral("正在编辑 %1 的任务；留空表示使用默认参数。应用后会重置仿真到初始态。")
                                      .arg(QString::fromStdString(selectedDefinition->identity.displayName.empty()
                                                                      ? selectedDefinition->identity.id
                                                                      : selectedDefinition->identity.displayName)));
    applyTaskButton_->setEnabled(true);
    updateTaskEditorFieldState();
}

void MainWindow::updateTaskEditorFieldState()
{
    const QString behavior = taskBehaviorComboBox_->currentData().toString();
    const bool hasSelection = !selectedEntityId_.empty() && applyTaskButton_->isEnabled();
    const bool interceptMode = behavior == QStringLiteral("intercept");
    const bool escortMode = behavior == QStringLiteral("escort");
    const bool orbitMode = behavior == QStringLiteral("orbit");
    const bool targetMode = interceptMode || escortMode || orbitMode;

    taskObjectiveEdit_->setEnabled(hasSelection);
    taskBehaviorComboBox_->setEnabled(hasSelection);
    taskTargetComboBox_->setEnabled(hasSelection && targetMode);
    taskInterceptDistanceEdit_->setEnabled(hasSelection && interceptMode);
    taskOrbitRadiusEdit_->setEnabled(hasSelection && orbitMode);
    taskOrbitDirectionComboBox_->setEnabled(hasSelection && orbitMode);
    taskOrbitAcquireToleranceEdit_->setEnabled(hasSelection && orbitMode);
    taskOrbitCompletionToleranceEdit_->setEnabled(hasSelection && orbitMode);
    taskOrbitHoldEdit_->setEnabled(hasSelection && orbitMode);
    taskEscortTrailEdit_->setEnabled(hasSelection && escortMode);
    taskEscortOffsetEdit_->setEnabled(hasSelection && escortMode);
    taskEscortSlotToleranceEdit_->setEnabled(hasSelection && escortMode);
    taskEscortHoldEdit_->setEnabled(hasSelection && escortMode);
}

bool MainWindow::applyTaskEditorChanges()
{
    if (selectedEntityId_.empty()) {
        return false;
    }

    const auto* scenario = session_.scenarioDefinition();
    if (scenario == nullptr) {
        return false;
    }

    const fm::app::EntityDefinition* selectedDefinition = nullptr;
    for (const auto& entity : scenario->entities) {
        if (entity.identity.id == selectedEntityId_) {
            selectedDefinition = &entity;
            break;
        }
    }

    if (selectedDefinition == nullptr) {
        return false;
    }

    auto parseOptionalDouble = [this](QLineEdit* edit, const QString& fieldName, bool nonNegative, std::optional<double>& target) -> bool {
        const QString text = edit->text().trimmed();
        if (text.isEmpty()) {
            target.reset();
            return true;
        }

        bool ok = false;
        const double value = text.toDouble(&ok);
        if (!ok) {
            QMessageBox::warning(this, QStringLiteral("任务参数无效"),
                                 QStringLiteral("字段“%1”需要填写数字，或留空使用默认值。").arg(fieldName));
            return false;
        }

        if (nonNegative && value < 0.0) {
            QMessageBox::warning(this, QStringLiteral("任务参数无效"),
                                 QStringLiteral("字段“%1”不能为负数。").arg(fieldName));
            return false;
        }

        target = value;
        return true;
    };

    fm::app::EntityMissionDefinition missionDefinition = selectedDefinition->mission;
    missionDefinition.objective = taskObjectiveEdit_->text().trimmed().toStdString();
    missionDefinition.behavior = taskBehaviorComboBox_->currentData().toString().toStdString();
    missionDefinition.targetEntityId = taskTargetComboBox_->isEnabled()
                                           ? taskTargetComboBox_->currentData().toString().toStdString()
                                           : std::string {};

    if (!parseOptionalDouble(taskInterceptDistanceEdit_, QStringLiteral("拦截完成距离"), true, missionDefinition.parameters.interceptCompletionDistanceMeters)
        || !parseOptionalDouble(taskOrbitRadiusEdit_, QStringLiteral("盘旋半径"), true, missionDefinition.parameters.orbitRadiusMeters)
        || !parseOptionalDouble(taskOrbitAcquireToleranceEdit_, QStringLiteral("盘旋进入容差"), true, missionDefinition.parameters.orbitAcquireToleranceMeters)
        || !parseOptionalDouble(taskOrbitCompletionToleranceEdit_, QStringLiteral("盘旋完成容差"), true, missionDefinition.parameters.orbitCompletionToleranceMeters)
        || !parseOptionalDouble(taskOrbitHoldEdit_, QStringLiteral("盘旋保持时间"), true, missionDefinition.parameters.orbitCompletionHoldSeconds)
        || !parseOptionalDouble(taskEscortTrailEdit_, QStringLiteral("护航尾随距离"), true, missionDefinition.parameters.escortTrailDistanceMeters)
        || !parseOptionalDouble(taskEscortOffsetEdit_, QStringLiteral("护航侧向偏移"), false, missionDefinition.parameters.escortRightOffsetMeters)
        || !parseOptionalDouble(taskEscortSlotToleranceEdit_, QStringLiteral("护航槽位容差"), true, missionDefinition.parameters.escortSlotToleranceMeters)
        || !parseOptionalDouble(taskEscortHoldEdit_, QStringLiteral("护航稳定保持"), true, missionDefinition.parameters.escortCompletionHoldSeconds)) {
        return false;
    }

    const QString orbitDirection = taskOrbitDirectionComboBox_->currentData().toString();
    if (orbitDirection == QStringLiteral("clockwise")) {
        missionDefinition.parameters.orbitClockwise = true;
    } else if (orbitDirection == QStringLiteral("counterclockwise")) {
        missionDefinition.parameters.orbitClockwise = false;
    }

    timer_->stop();
    replaySnapshotIndex_ = -1;
    if (!session_.updateEntityMissionDefinition(selectedEntityId_, std::move(missionDefinition))) {
        QMessageBox::warning(this, QStringLiteral("任务应用失败"), session_.lastError());
        return false;
    }

    forceFitViewOnNextRefresh_ = true;
    updateEntityList();
    updateTaskEditor();
    updateEntityDetails();
    updateRecordingSummary();
    updateLogView();
    refreshScene();
    updateStatus();
    return true;
}

void MainWindow::updateEntityDetails()
{
    const auto states = currentRenderStates();
    for (const auto& state : states) {
        if (state.id == selectedEntityId_) {
            const auto trail = session_.trajectoryForEntity(state.id);
            QString detectedTargets = QStringLiteral("无");
            if (!state.detectedTargetIds.empty()) {
                QStringList targetIds;
                for (const auto& id : state.detectedTargetIds) {
                    targetIds.push_back(QString::fromStdString(id));
                }

                detectedTargets = targetIds.join(QStringLiteral(", "));
            }

            const QString missionBehavior = formatMissionBehaviorText(state.missionBehavior);
            const QString missionTarget = state.missionTargetEntityId.empty()
                                              ? QStringLiteral("无")
                                              : QString::fromStdString(state.missionTargetEntityId);
            const QString taskParameters = state.missionTaskParametersSummary.empty()
                                               ? QStringLiteral("默认")
                                               : QString::fromStdString(state.missionTaskParametersSummary);
            const QString executionStatus = formatMissionExecutionStatus(state.missionExecutionStatus);
            const QString executionPhase = formatMissionExecutionPhase(state.missionExecutionPhase);
            const QString terminalReason = formatMissionTerminalReason(state.missionTerminalReason);
            const QString prerequisiteEntity = state.missionPrerequisiteEntityId.empty()
                                                   ? QStringLiteral("无")
                                                   : QString::fromStdString(state.missionPrerequisiteEntityId);
            const QString prerequisiteStatus = state.missionPrerequisiteStatus.empty()
                                                   ? QStringLiteral("无")
                                                   : formatMissionExecutionStatus(state.missionPrerequisiteStatus);
            const QString timeoutText = state.missionTimeoutSeconds > 0.0
                                            ? QStringLiteral("%1 s").arg(state.missionTimeoutSeconds, 0, 'f', 1)
                                            : QStringLiteral("无");
            const QString replanBehavior = state.missionReplanBehavior.empty()
                                               ? QStringLiteral("无")
                                               : QString::fromStdString(state.missionReplanBehavior);
            const QString lastReplanReason = state.missionLastReplanReason.empty()
                                                ? QStringLiteral("无")
                                                : formatMissionTerminalReason(state.missionLastReplanReason);
            const QString lastReplanTransition = state.missionLastReplanReason.empty()
                                        ? QStringLiteral("无")
                                        : QStringLiteral("%1 -> %2")
                                            .arg(state.missionLastReplanFromBehavior.empty()
                                                 ? QStringLiteral("无")
                                                 : QString::fromStdString(state.missionLastReplanFromBehavior))
                                            .arg(state.missionLastReplanToBehavior.empty()
                                                 ? QStringLiteral("无")
                                                 : QString::fromStdString(state.missionLastReplanToBehavior));
            const QString guidanceMode = formatMovementGuidanceMode(state.movementGuidanceMode);
            const QString guidanceTarget = state.movementGuidanceTargetName.empty()
                                               ? QStringLiteral("无")
                                               : QString::fromStdString(state.movementGuidanceTargetName);
            const QString activeWaypoint = formatActiveWaypoint(state);
            const QString loiterState = state.routeIsLoitering
                                            ? QStringLiteral("是，剩余 %1 s").arg(state.routeLoiterSecondsRemaining, 0, 'f', 1)
                                            : QStringLiteral("否");
            const QString lastCompletedWaypoint = state.missionLastCompletedWaypointName.empty()
                                                     ? QStringLiteral("无")
                                                     : QString::fromStdString(state.missionLastCompletedWaypointName);

            entityDetailsLabel_->setText(
                QStringLiteral("ID: %1\n名称: %2\n阵营: %3\n类型: %4\n角色: %5\n标签: %6\n位置: (%7, %8)\n速度: (%9, %10)\n航向: %11°\n最大速度: %12 m/s\n最大转率: %13 °/s\n轨迹点数: %14\n传感器半径: %15 m\n传感器视场: %16°\n机动引导: %17\n引导目标: %18\n任务目标: %19\n任务行为: %20\n任务对象: %21\n任务参数: %22\n任务状态: %23\n执行阶段: %24\n终态原因: %25\n前置任务实体: %26\n前置任务状态: %27\n前置约束满足: %28\n任务超时: %29\n下一回退任务: %30\n剩余回退步数: %31\n最近重规划原因: %32\n最近重规划切换: %33\n阶段持续: %34 s\n阶段起始 Tick: %35\n已到达航点数: %36\n已完成航路圈数: %37\n最近到达航点: %38\n当前航路点: %39\n航路点等待: %40\n机动航路:\n%41\n探测目标: %42")
                    .arg(QString::fromStdString(state.id))
                    .arg(QString::fromStdString(state.displayName))
                    .arg(state.side.empty() ? QStringLiteral("未指定") : QString::fromStdString(state.side))
                    .arg(state.category.empty() ? QStringLiteral("未指定") : QString::fromStdString(state.category))
                    .arg(state.role.empty() ? QStringLiteral("未指定") : QString::fromStdString(state.role))
                    .arg(formatEntityTags(state.tags))
                    .arg(state.position.x, 0, 'f', 2)
                    .arg(state.position.y, 0, 'f', 2)
                    .arg(state.velocity.x, 0, 'f', 2)
                    .arg(state.velocity.y, 0, 'f', 2)
                    .arg(state.headingDegrees, 0, 'f', 1)
                    .arg(state.maxSpeedMetersPerSecond, 0, 'f', 1)
                    .arg(state.maxTurnRateDegreesPerSecond, 0, 'f', 1)
                    .arg(trail.size())
                    .arg(state.sensorRangeMeters, 0, 'f', 1)
                    .arg(state.sensorFieldOfViewDegrees, 0, 'f', 1)
                    .arg(guidanceMode)
                    .arg(guidanceTarget)
                    .arg(state.missionObjective.empty() ? QStringLiteral("无") : QString::fromStdString(state.missionObjective))
                    .arg(missionBehavior)
                    .arg(missionTarget)
                    .arg(taskParameters)
                    .arg(executionStatus)
                    .arg(executionPhase)
                    .arg(terminalReason)
                    .arg(prerequisiteEntity)
                    .arg(prerequisiteStatus)
                    .arg(state.missionActivationSatisfied ? QStringLiteral("是") : QStringLiteral("否"))
                    .arg(timeoutText)
                    .arg(replanBehavior)
                    .arg(state.missionRemainingReplanSteps)
                    .arg(lastReplanReason)
                    .arg(lastReplanTransition)
                    .arg(state.missionPhaseElapsedSeconds, 0, 'f', 2)
                    .arg(state.missionPhaseEnteredTick)
                    .arg(state.missionCompletedWaypointVisits)
                    .arg(state.missionCompletedRouteCycles)
                    .arg(lastCompletedWaypoint)
                    .arg(activeWaypoint)
                    .arg(loiterState)
                    .arg(formatRouteWaypoints(state.routeWaypoints))
                    .arg(detectedTargets));
            return;
        }
    }

    entityDetailsLabel_->setText(QStringLiteral("当前没有可显示的实体信息。"));
}

void MainWindow::updateLogView()
{
    QStringList lines;
    const QString filterKey = logFilterComboBox_ != nullptr
                                  ? logFilterComboBox_->currentData().toString()
                                  : QStringLiteral("all");
    const auto states = currentRenderStates();
    const bool onlySelectedEntityLogs = selectedEntityLogOnlyCheckBox_ != nullptr
                                        && selectedEntityLogOnlyCheckBox_->isChecked();
    for (const auto& entry : session_.eventLog()) {
        if (matchesLogFilter(entry, filterKey)
            && (!onlySelectedEntityLogs || matchesSelectedEntityLog(entry, selectedEntityId_, states))) {
            lines.push_back(entry);
        }
    }

    logView_->setPlainText(lines.join(QStringLiteral("\n")));
    auto* scrollBar = logView_->verticalScrollBar();
    if (scrollBar != nullptr) {
        scrollBar->setValue(scrollBar->maximum());
    }
}

void MainWindow::updateRecordingSummary()
{
    const auto* snapshot = currentSnapshot();
    if (snapshot == nullptr) {
        recordingSummaryLabel_->setText(QStringLiteral("当前没有记录数据。"));
        return;
    }

    std::size_t detectedCount = 0;
    std::size_t activeMissionCount = 0;
    std::size_t completedMissionCount = 0;
    std::size_t abortedMissionCount = 0;
    for (const auto& entity : snapshot->entities) {
        detectedCount += entity.detectedTargetIds.size();
    }

    for (const auto& state : currentRenderStates()) {
        if (state.missionExecutionStatus == "completed") {
            ++completedMissionCount;
        } else if (state.missionExecutionStatus == "aborted") {
            ++abortedMissionCount;
        } else if (!state.missionExecutionStatus.empty() && state.missionExecutionStatus != "unassigned") {
            ++activeMissionCount;
        }
    }

    recordingSummaryLabel_->setText(
        QStringLiteral("模式: %1\n快照总数: %2\n当前 Tick: %3\n当前仿真时间: %4 s\n记录实体数: %5\n活动任务数: %6\n已完成任务数: %7\n已中止任务数: %8\n探测结果数: %9")
            .arg(isReplayMode() ? QStringLiteral("回放") : QStringLiteral("实时"))
            .arg(session_.recording().size())
            .arg(snapshot->tickCount)
            .arg(snapshot->elapsedSimulationSeconds, 0, 'f', 2)
            .arg(snapshot->entities.size())
            .arg(activeMissionCount)
            .arg(completedMissionCount)
            .arg(abortedMissionCount)
            .arg(detectedCount));
}

void MainWindow::updateStatus()
{
    const auto* snapshot = currentSnapshot();
    const auto displayTime = snapshot != nullptr ? snapshot->elapsedSimulationSeconds : session_.elapsedSimulationSeconds();
    const auto displayTick = snapshot != nullptr ? snapshot->tickCount : session_.tickCount();

    if (session_.hasScenario()) {
        const auto& environment = session_.currentScenarioEnvironment();
        const auto& bounds = session_.currentScenarioMapBounds();
        const QString description = session_.currentScenarioDescription().isEmpty()
                                        ? QStringLiteral("无描述")
                                        : session_.currentScenarioDescription();
        scenarioLabel_->setText(
            QStringLiteral("当前场景: %1\n描述: %2\n环境: %3 / %4 / 可视距离 %5 m / 风 (%6, %7)\n边界: (%8, %9) -> (%10, %11)")
                .arg(session_.currentScenarioName())
                .arg(description)
                .arg(QString::fromStdString(environment.timeOfDay.empty() ? std::string("unknown") : environment.timeOfDay))
                .arg(QString::fromStdString(environment.weather.empty() ? std::string("clear") : environment.weather))
                .arg(environment.visibilityMeters, 0, 'f', 0)
                .arg(environment.wind.x, 0, 'f', 1)
                .arg(environment.wind.y, 0, 'f', 1)
                .arg(bounds.minimum.x, 0, 'f', 1)
                .arg(bounds.minimum.y, 0, 'f', 1)
                .arg(bounds.maximum.x, 0, 'f', 1)
                .arg(bounds.maximum.y, 0, 'f', 1));
    } else {
        scenarioLabel_->setText(QStringLiteral("当前场景: 未加载"));
    }

    statusLabel_->setText(QStringLiteral("状态: %1 | 视图: %2 | 仿真时间: %3 s | Tick: %4 | 时间步: %5 s")
                              .arg(toStateText(session_.state()))
                              .arg(isReplayMode() ? QStringLiteral("回放") : QStringLiteral("实时"))
                              .arg(displayTime, 0, 'f', 2)
                              .arg(displayTick)
                              .arg(session_.fixedTimeStepSeconds(), 0, 'f', 3));
}

}  // namespace fm::ui
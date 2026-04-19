#include "gui/app.hpp"

#include "ga/ga.hpp"
#include "ga/population.hpp"
#include "ga/problem.hpp"
#include "ga/rng.hpp"
#include "gui/platform.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <imgui.h>
#include <implot.h>
#include <limits>
#include <memory>
#include <numeric>
#include <random>
#include <string>
#include <vector>

namespace gui {

static constexpr int kGhostDepth = 8;
static constexpr int kCurvePoints = 200;
static constexpr float kConvWindow = 50.0f; // generations visible while auto-follow is on
static constexpr float kLeftPanelW = 340.0f;

// heatmap cell encoding
// doubles because ImPlot::PlotHeatmap takes a const double* buffer
static constexpr double kBitUnchanged0 = 0.0;  // bit was 0, still 0
static constexpr double kBitFlipped1to0 = 1.0; // bit was 1 before mutation, now 0
static constexpr double kBitFlipped0to1 = 2.0; // bit was 0 before mutation, now 1
static constexpr double kBitUnchanged1 = 3.0;  // bit was 1, still 1

// color palette
namespace col {
// backgrounds
constexpr ImVec4 crust = {0.067f, 0.067f, 0.106f, 1.0f};   // #11111B
constexpr ImVec4 mantle = {0.094f, 0.094f, 0.145f, 1.0f};  // #181825
constexpr ImVec4 base = {0.118f, 0.118f, 0.180f, 1.0f};    // #1E1E2E
constexpr ImVec4 surf0 = {0.192f, 0.196f, 0.267f, 1.0f};   // #313244
constexpr ImVec4 surf1 = {0.271f, 0.278f, 0.353f, 1.0f};   // #45475A
constexpr ImVec4 surf2 = {0.345f, 0.357f, 0.439f, 1.0f};   // #585B70
constexpr ImVec4 overlay = {0.424f, 0.439f, 0.525f, 1.0f}; // #6C7086
// text
constexpr ImVec4 subtext = {0.651f, 0.678f, 0.784f, 1.0f}; // #A6ADC8
constexpr ImVec4 text = {0.804f, 0.839f, 0.957f, 1.0f};    // #CDD6F4
// accent colours
constexpr ImVec4 blue = {0.537f, 0.706f, 0.980f, 1.0f};     // #89B4FA
constexpr ImVec4 lavender = {0.706f, 0.745f, 0.996f, 1.0f}; // #B4BEFE
constexpr ImVec4 teal = {0.580f, 0.886f, 0.835f, 1.0f};     // #94E2D5
constexpr ImVec4 green = {0.651f, 0.890f, 0.631f, 1.0f};    // #A6E3A1
constexpr ImVec4 yellow = {0.976f, 0.886f, 0.686f, 1.0f};   // #F9E2AF
constexpr ImVec4 peach = {0.980f, 0.702f, 0.529f, 1.0f};    // #FAB387
constexpr ImVec4 red = {0.953f, 0.545f, 0.659f, 1.0f};      // #F38BA8
constexpr ImVec4 mauve = {0.796f, 0.651f, 0.969f, 1.0f};    // #CBA6F7
} // namespace col

static void applyTheme() {
    // ImGui geometry
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowPadding = {12, 10};
    s.FramePadding = {8, 4};
    s.ItemSpacing = {8, 6};
    s.ItemInnerSpacing = {6, 4};
    s.IndentSpacing = 16;
    s.ScrollbarSize = 10;
    s.GrabMinSize = 10;

    s.WindowBorderSize = 0;
    s.ChildBorderSize = 1;
    s.FrameBorderSize = 0;
    s.PopupBorderSize = 1;

    s.WindowRounding = 8;
    s.ChildRounding = 6;
    s.FrameRounding = 6;
    s.PopupRounding = 6;
    s.ScrollbarRounding = 8;
    s.GrabRounding = 4;
    s.TabRounding = 4;

    s.AntiAliasedLinesUseTex = false;

    // ImGui
    ImVec4* c = s.Colors;
    c[ImGuiCol_Text] = col::text;
    c[ImGuiCol_TextDisabled] = col::subtext;
    c[ImGuiCol_WindowBg] = col::base;
    c[ImGuiCol_ChildBg] = {col::surf0.x, col::surf0.y, col::surf0.z, 0.55f};
    c[ImGuiCol_PopupBg] = col::surf0;
    c[ImGuiCol_Border] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.70f};
    c[ImGuiCol_BorderShadow] = {0, 0, 0, 0};
    c[ImGuiCol_FrameBg] = {col::surf0.x, col::surf0.y, col::surf0.z, 0.80f};
    c[ImGuiCol_FrameBgHovered] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.80f};
    c[ImGuiCol_FrameBgActive] = {col::surf2.x, col::surf2.y, col::surf2.z, 0.80f};
    c[ImGuiCol_TitleBg] = col::mantle;
    c[ImGuiCol_TitleBgActive] = col::surf0;
    c[ImGuiCol_TitleBgCollapsed] = col::mantle;
    c[ImGuiCol_MenuBarBg] = col::surf0;
    c[ImGuiCol_ScrollbarBg] = {col::mantle.x, col::mantle.y, col::mantle.z, 0.50f};
    c[ImGuiCol_ScrollbarGrab] = col::surf2;
    c[ImGuiCol_ScrollbarGrabHovered] = col::overlay;
    c[ImGuiCol_ScrollbarGrabActive] = col::blue;
    c[ImGuiCol_CheckMark] = col::blue;
    c[ImGuiCol_SliderGrab] = col::blue;
    c[ImGuiCol_SliderGrabActive] = col::lavender;
    c[ImGuiCol_Button] = {col::blue.x, col::blue.y, col::blue.z, 0.20f};
    c[ImGuiCol_ButtonHovered] = {col::blue.x, col::blue.y, col::blue.z, 0.45f};
    c[ImGuiCol_ButtonActive] = {col::blue.x, col::blue.y, col::blue.z, 0.70f};
    c[ImGuiCol_Header] = {col::blue.x, col::blue.y, col::blue.z, 0.18f};
    c[ImGuiCol_HeaderHovered] = {col::blue.x, col::blue.y, col::blue.z, 0.35f};
    c[ImGuiCol_HeaderActive] = {col::blue.x, col::blue.y, col::blue.z, 0.55f};
    c[ImGuiCol_Separator] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.80f};
    c[ImGuiCol_SeparatorHovered] = col::blue;
    c[ImGuiCol_SeparatorActive] = col::lavender;
    c[ImGuiCol_ResizeGrip] = {col::blue.x, col::blue.y, col::blue.z, 0.18f};
    c[ImGuiCol_ResizeGripHovered] = {col::blue.x, col::blue.y, col::blue.z, 0.45f};
    c[ImGuiCol_ResizeGripActive] = {col::blue.x, col::blue.y, col::blue.z, 0.75f};
    c[ImGuiCol_PlotLines] = col::teal;
    c[ImGuiCol_PlotLinesHovered] = col::lavender;
    c[ImGuiCol_PlotHistogram] = col::blue;
    c[ImGuiCol_PlotHistogramHovered] = col::lavender;
    c[ImGuiCol_TextSelectedBg] = {col::blue.x, col::blue.y, col::blue.z, 0.35f};
    c[ImGuiCol_DragDropTarget] = col::teal;
    c[ImGuiCol_NavHighlight] = col::blue;
    c[ImGuiCol_NavWindowingHighlight] = col::blue;
    c[ImGuiCol_NavWindowingDimBg] = {col::base.x, col::base.y, col::base.z, 0.70f};
    c[ImGuiCol_ModalWindowDimBg] = {col::base.x, col::base.y, col::base.z, 0.50f};

    // ImPlot
    ImPlotStyle& ps = ImPlot::GetStyle();
    ps.PlotPadding = {10, 8};
    ps.LabelPadding = {6, 4};
    ps.LegendPadding = {8, 6};
    ps.LegendInnerPadding = {4, 3};
    ps.LegendSpacing = {4, 2};
    ps.PlotBorderSize = 1.0f;
    ps.MinorAlpha = 0.12f;

    ImVec4* pc = ps.Colors;
    pc[ImPlotCol_FrameBg] = {col::surf0.x, col::surf0.y, col::surf0.z, 0.40f};
    pc[ImPlotCol_PlotBg] = col::crust;
    pc[ImPlotCol_PlotBorder] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.90f};
    pc[ImPlotCol_LegendBg] = {col::surf0.x, col::surf0.y, col::surf0.z, 0.90f};
    pc[ImPlotCol_LegendBorder] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.90f};
    pc[ImPlotCol_LegendText] = col::text;
    pc[ImPlotCol_TitleText] = col::subtext;
    pc[ImPlotCol_InlayText] = col::subtext;
    pc[ImPlotCol_AxisText] = col::subtext;
    pc[ImPlotCol_AxisGrid] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.35f};
    pc[ImPlotCol_AxisTick] = {col::surf2.x, col::surf2.y, col::surf2.z, 0.60f};
    pc[ImPlotCol_AxisBg] = {0, 0, 0, 0};
    pc[ImPlotCol_AxisBgHovered] = {col::surf1.x, col::surf1.y, col::surf1.z, 0.20f};
    pc[ImPlotCol_AxisBgActive] = {col::surf2.x, col::surf2.y, col::surf2.z, 0.30f};
    pc[ImPlotCol_Selection] = {col::teal.x, col::teal.y, col::teal.z, 0.55f};
    pc[ImPlotCol_Crosshairs] = {col::surf2.x, col::surf2.y, col::surf2.z, 0.80f};
}

struct Snapshot {
    std::vector<float> xs, ys;
};

class GuiApp {
  public:
    GuiApp(const GaParams& baseline, uint64_t seed);
    void frame();

  private:
    uint64_t seed_;
    GaParams baselineParams_; // restored on Reset; never mutated after ctor
    GaParams editParams_;     // what the parameter panel widgets write into

    std::unique_ptr<Rng> rng_;
    std::unique_ptr<Ga> ga_;

    GenerationTrace lastTrace_;
    std::vector<float> maxHistory_;
    std::vector<float> meanHistory_;
    double bestEverFitness_ = -std::numeric_limits<double>::infinity();
    double bestEverX_ = 0.0;

    std::deque<Snapshot> ghost_;

    bool playing_ = false;
    bool autoFollow_ = true;
    float gensPerSec_ = 5.0f;
    double accumulator_ = 0.0;

    int scatterResetFrames_ = 0;
    int convResetFrames_ = 0;

    std::string errorMsg_;

    std::vector<float> fxCurveX_;
    std::vector<float> fxCurveY_;

    std::vector<double> heatmapData_;
    std::vector<int> sortedRows_;

    ImPlotColormap genomeCmap_ = 0;

    // rng for UI-driven randomisation; kept separate from the GA's reproducible rng
    std::mt19937 uiRng_{std::random_device{}()};

    int currentGen_() const { return ga_ ? ga_->currentGeneration() : 0; }
    int maxGen_() const { return editParams_.generations; }

    void doStep_();
    void rebuild_(const GaParams& p);
    void cacheFxCurve_(const GaParams& p);
    void pushGhost_();
    void rebuildHeatmap_();
    void randomizeCoefficients_();
    void handleShortcuts_();

    void drawParamsPanel_();
    void drawScatterPanel_(float h);
    void drawHistoryPanel_(float h);
    void drawHeatmapPanel_(float h);
    void drawToolbar_(float h);
};

// construction

GuiApp::GuiApp(const GaParams& baseline, uint64_t seed)
    : seed_(seed), baselineParams_(baseline), editParams_(baseline) {
    // qualitative (qual=true) so each cell value maps to exactly one colour bin, no interpolation
    // order matches the kBit* constants: unchanged-0, flipped 1→0, flipped 0→1, unchanged-1
    ImVec4 palette[4] = {col::mantle, col::red, col::green, col::teal};
    genomeCmap_ = ImPlot::AddColormap("##genome_bits", palette, 4, true);

    rebuild_(baseline);
}

// per-step helpers

void GuiApp::doStep_() {
    lastTrace_ = GenerationTrace{};
    try {
        ga_->step(&lastTrace_);
    } catch (const std::exception& e) {
        errorMsg_ = e.what();
        playing_ = false;
        return;
    }
    maxHistory_.push_back((float)lastTrace_.stats.maxFitness);
    meanHistory_.push_back((float)lastTrace_.stats.meanFitness);
    if (lastTrace_.stats.maxFitness > bestEverFitness_) {
        bestEverFitness_ = lastTrace_.stats.maxFitness;
        bestEverX_ = lastTrace_.afterMutation[lastTrace_.stats.bestIndex].x;
    }
    pushGhost_();
    rebuildHeatmap_();
}

void GuiApp::rebuild_(const GaParams& p) {
    errorMsg_.clear();
    ga_.reset();
    rng_.reset();
    maxHistory_.clear();
    meanHistory_.clear();
    ghost_.clear();
    heatmapData_.clear();
    sortedRows_.clear();
    lastTrace_ = GenerationTrace{};
    bestEverFitness_ = -std::numeric_limits<double>::infinity();
    bestEverX_ = 0.0;
    playing_ = false;
    accumulator_ = 0.0;
    // reset the view on next frame
    // doing it twice because the first reset sometimes happens before the data is ready
    scatterResetFrames_ = 2;
    convResetFrames_ = 2;

    try {
        rng_ = std::make_unique<Rng>(seed_);
        ga_ = std::make_unique<Ga>(p, *rng_);
        cacheFxCurve_(p);
    } catch (const std::exception& e) {
        errorMsg_ = e.what();
    }
}

void GuiApp::cacheFxCurve_(const GaParams& p) {
    // recompute the 200-sample f(x) curve whenever params change
    fxCurveX_.resize(kCurvePoints);
    fxCurveY_.resize(kCurvePoints);
    Problem prob = Problem::make(p.a, p.b, p.A, p.B, p.C, p.precision);
    for (int i = 0; i < kCurvePoints; ++i) {
        double t = (double)i / (kCurvePoints - 1);
        double x = p.a + t * (p.b - p.a);
        fxCurveX_[i] = (float)x;
        fxCurveY_[i] = (float)prob.fitness(x);
    }
}

void GuiApp::pushGhost_() {
    Snapshot s;
    int n = lastTrace_.afterMutation.size();
    s.xs.reserve(n);
    s.ys.reserve(n);
    for (int i = 0; i < n; ++i) {
        s.xs.push_back((float)lastTrace_.afterMutation[i].x);
        s.ys.push_back((float)lastTrace_.afterMutation[i].fitness);
    }
    ghost_.push_back(std::move(s));
    if ((int)ghost_.size() > kGhostDepth)
        ghost_.pop_front();
}

void GuiApp::rebuildHeatmap_() {
    if (!ga_)
        return;
    const Population& pop = lastTrace_.afterMutation;
    const Population& selPop = lastTrace_.afterSelection;
    int n = pop.size();
    if (n == 0)
        return;
    int l = ga_->problem().bitLength;

    // rows sorted best-first so the top row is always the elite
    sortedRows_.resize(n);
    std::iota(sortedRows_.begin(), sortedRows_.end(), 0);
    std::sort(sortedRows_.begin(), sortedRows_.end(),
              [&](int a, int b) { return pop[a].fitness > pop[b].fitness; });

    heatmapData_.assign((size_t)(n * l), kBitUnchanged0);
    for (int r = 0; r < n; ++r) {
        int idx = sortedRows_[r];
        for (int j = 0; j < l; ++j) {
            bool selBit = selPop[idx].bits[j]; // pre-mutation
            bool mutBit = pop[idx].bits[j];    // post-mutation
            double v;
            if (selBit == mutBit)
                v = mutBit ? kBitUnchanged1 : kBitUnchanged0;
            else
                v = mutBit ? kBitFlipped0to1 : kBitFlipped1to0;
            heatmapData_[(size_t)(r * l + j)] = v;
        }
    }
}

// pick a new A, B, C that produce a visible max on the current [a, b] and keep fitness
// strictly positive on the whole domain
void GuiApp::randomizeCoefficients_() {
    std::uniform_real_distribution<double> dA(-2.0, -0.3); // down-opening, non-degenerate
    std::uniform_real_distribution<double> dB(-3.0, 3.0);
    double A = dA(uiRng_);
    double B = dB(uiRng_);
    // we want f(x) >= 1 on the endpoints; A<0 means the minimum on [a,b] is at an endpoint
    double fa = A * editParams_.a * editParams_.a + B * editParams_.a;
    double fb = A * editParams_.b * editParams_.b + B * editParams_.b;
    double minAtEnd = std::min(fa, fb);
    double C = 1.0 - minAtEnd;

    editParams_.A = A;
    editParams_.B = B;
    editParams_.C = C;
    rebuild_(editParams_);
}

// panels

void GuiApp::drawParamsPanel_() {
    ImGui::Text("Parameters");
    ImGui::Separator();

    float avail = ImGui::GetContentRegionAvail().x;
    ImGui::PushItemWidth(avail * 0.55f);

    ImGui::InputInt("n (pop size)", &editParams_.populationSize);
    if (editParams_.populationSize < 2)
        editParams_.populationSize = 2;

    ImGui::InputDouble("a", &editParams_.a, 0.0, 0.0, "%.4f");
    ImGui::InputDouble("b", &editParams_.b, 0.0, 0.0, "%.4f");

    ImGui::Spacing();
    ImGui::TextDisabled("f(x) = A x^2 + B x + C");
    ImGui::InputDouble("A  (x^2 coeff)", &editParams_.A, 0.0, 0.0, "%.4f");
    ImGui::InputDouble("B  (x coeff)", &editParams_.B, 0.0, 0.0, "%.4f");
    ImGui::InputDouble("C  (constant)", &editParams_.C, 0.0, 0.0, "%.4f");
    if (ImGui::Button("Randomize f(x)", ImVec2(-1, 0)))
        randomizeCoefficients_();
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("pick new A, B, C (down-opening parabola, positive on [a,b])");

    ImGui::Spacing();
    ImGui::InputInt("precision (digits)", &editParams_.precision);
    if (editParams_.precision < 1)
        editParams_.precision = 1;

    ImGui::Spacing();
    auto pc = (float)editParams_.crossoverProb;
    auto pm = (float)editParams_.mutationProb;
    ImGui::SliderFloat("p_c", &pc, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("p_m", &pm, 0.0f, 1.0f, "%.4f");
    editParams_.crossoverProb = (double)pc;
    editParams_.mutationProb = (double)pm;

    ImGui::Spacing();
    ImGui::InputInt("generations T", &editParams_.generations);
    if (editParams_.generations < 1)
        editParams_.generations = 1;

    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::TextDisabled("seed: %llu", (unsigned long long)seed_);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
    if (ImGui::Button("Apply", ImVec2(btnW, 0)))
        rebuild_(editParams_);
    ImGui::SameLine();
    if (ImGui::Button("Reset", ImVec2(btnW, 0))) {
        editParams_ = baselineParams_;
        rebuild_(baselineParams_);
    }

    if (!errorMsg_.empty()) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Text, col::red);
        ImGui::TextWrapped("%s", errorMsg_.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    ImGui::TextDisabled("shortcuts");
    ImGui::BulletText("space: play / pause");
    ImGui::BulletText("right arrow: single step");
    ImGui::BulletText("r: reset run");
    ImGui::BulletText("g: randomize f(x)");
}

void GuiApp::drawScatterPanel_(float h) {
    ImPlotFlags flags = ImPlotFlags_NoLegend;
    if (!ImPlot::BeginPlot("fitness landscape", ImVec2(-1, h), flags))
        return;

    ImPlot::SetupAxis(ImAxis_X1, "x");
    ImPlot::SetupAxis(ImAxis_Y1, "f(x)");
    // only pin the view right after a rebuild; otherwise let the user pan/zoom both axes
    if (ga_ && scatterResetFrames_ > 0) {
        ImPlot::SetupAxisLimits(ImAxis_X1, editParams_.a, editParams_.b, ImPlotCond_Always);
        // y: padded range of the cached curve so we don't chop off the peak
        if (!fxCurveY_.empty()) {
            auto [mn, mx] = std::minmax_element(fxCurveY_.begin(), fxCurveY_.end());
            double pad = std::max(0.1, (*mx - *mn) * 0.10);
            ImPlot::SetupAxisLimits(ImAxis_Y1, *mn - pad, *mx + pad, ImPlotCond_Always);
        }
        --scatterResetFrames_;
    }

    // f(x) curve
    if (!fxCurveX_.empty()) {
        ImPlotSpec cs;
        cs.LineColor = col::green;
        cs.LineWeight = 1.5f;
        cs.FillColor = col::green;
        cs.FillAlpha = 0.07f;
        cs.Flags = ImPlotItemFlags_NoFit | ImPlotLineFlags_Shaded;
        ImPlot::PlotLine("##curve", fxCurveX_.data(), fxCurveY_.data(), kCurvePoints, cs);
    }

    // ghost trail: fading from 6% (oldest) to 28% (newest)
    int ng = (int)ghost_.size();
    for (int g = 0; g < ng; ++g) {
        float alpha = 0.06f + 0.22f * (float)(g + 1) / ng;
        ImPlotSpec gs;
        gs.Marker = ImPlotMarker_Circle;
        gs.MarkerSize = 3.0f;
        gs.MarkerFillColor = {col::lavender.x, col::lavender.y, col::lavender.z, alpha};
        gs.MarkerLineColor = {0, 0, 0, 0};
        gs.Flags = ImPlotItemFlags_NoFit | ImPlotItemFlags_NoLegend;
        const auto& snap = ghost_[g];
        ImPlot::PlotScatter("##ghost", snap.xs.data(), snap.ys.data(), (int)snap.xs.size(), gs);
    }

    // current population (after mutation)
    if (ga_ && currentGen_() > 0) {
        const Population& pop = lastTrace_.afterMutation;
        int n = pop.size();
        int eliteIdx = lastTrace_.stats.bestIndex;

        if (n <= 0 || eliteIdx < 0 || eliteIdx >= n) {
            ImPlot::EndPlot();
            return;
        }

        // non-elite
        std::vector<float> xs, ys;
        xs.reserve(n - 1);
        ys.reserve(n - 1);
        for (int i = 0; i < n; ++i) {
            if (i == eliteIdx)
                continue;
            xs.push_back((float)pop[i].x);
            ys.push_back((float)pop[i].fitness);
        }
        if (!xs.empty()) {
            ImPlotSpec ps;
            ps.Marker = ImPlotMarker_Circle;
            ps.MarkerSize = 4.5f;
            ps.MarkerFillColor = {col::blue.x, col::blue.y, col::blue.z, 0.80f};
            ps.MarkerLineColor = {col::text.x, col::text.y, col::text.z, 0.25f};
            ps.LineWeight = 0.8f;
            ps.Flags = ImPlotItemFlags_NoLegend;
            ImPlot::PlotScatter("##pop", xs.data(), ys.data(), (int)xs.size(), ps);
        }

        // elite
        float ex = (float)pop[eliteIdx].x;
        float ey = (float)pop[eliteIdx].fitness;
        ImPlotSpec es;
        es.Marker = ImPlotMarker_Asterisk;
        es.MarkerSize = 10.0f;
        es.MarkerLineColor = col::yellow;
        es.LineWeight = 2.2f;
        es.Flags = ImPlotItemFlags_NoLegend;
        ImPlot::PlotScatter("##elite", &ex, &ey, 1, es);

        // best-ever marker
        if (bestEverFitness_ > -std::numeric_limits<double>::infinity()) {
            float bx = (float)bestEverX_;
            float by = (float)bestEverFitness_;
            ImPlotSpec bs;
            bs.Marker = ImPlotMarker_Diamond;
            bs.MarkerSize = 8.0f;
            bs.MarkerFillColor = {0, 0, 0, 0};
            bs.MarkerLineColor = col::green;
            bs.LineWeight = 2.0f;
            bs.Flags = ImPlotItemFlags_NoFit | ImPlotItemFlags_NoLegend;
            ImPlot::PlotScatter("##bestever", &bx, &by, 1, bs);
        }
    }

    ImPlot::EndPlot();
}

void GuiApp::drawHistoryPanel_(float h) {
    if (!ImPlot::BeginPlot("convergence", ImVec2(-1, h)))
        return;

    ImPlot::SetupAxis(ImAxis_X1, "generation");
    ImPlot::SetupAxis(ImAxis_Y1, "fitness");

    int n = (int)maxHistory_.size();

    // scroll the X window while playing
    if (autoFollow_ && (playing_ || convResetFrames_ > 0)) {
        float gen = (float)currentGen_();
        float left = gen - 0.75f * kConvWindow;
        float right = gen + 0.25f * kConvWindow;
        if (left < 0.0f) {
            left = 0.0f;
            right = kConvWindow;
        }
        ImPlot::SetupAxisLimits(ImAxis_X1, left, right, ImPlotCond_Always);
    } else if (convResetFrames_ > 0) {
        ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, (double)kConvWindow, ImPlotCond_Always);
    }

    // fit Y every frame while playing so the max line stays in frame
    if (n > 0 && (playing_ || convResetFrames_ > 0)) {
        float ymin = *std::min_element(meanHistory_.begin(), meanHistory_.end());
        float ymax = *std::max_element(maxHistory_.begin(), maxHistory_.end());
        float pad = std::max(0.05f * (ymax - ymin), 0.02f);
        ImPlot::SetupAxisLimits(ImAxis_Y1, ymin - pad, ymax + pad, ImPlotCond_Always);
    }
    if (convResetFrames_ > 0)
        --convResetFrames_;

    if (n > 0) {
        ImPlotSpec maxSpec;
        maxSpec.LineColor = col::peach;
        maxSpec.LineWeight = 2.0f;
        maxSpec.FillColor = col::peach;
        maxSpec.FillAlpha = 0.10f;
        maxSpec.Flags = ImPlotLineFlags_Shaded;
        ImPlot::PlotLine("max", maxHistory_.data(), n, 1.0, 1.0, maxSpec);

        ImPlotSpec meanSpec;
        meanSpec.LineColor = col::teal;
        meanSpec.LineWeight = 1.5f;
        ImPlot::PlotLine("mean", meanHistory_.data(), n, 1.0, 1.0, meanSpec);
    }

    ImPlot::EndPlot();
}

void GuiApp::drawHeatmapPanel_(float h) {
    // genome: n rows (chromosomes, best-first) * l cols (bits from MSB to LSB)
    if (ImGui::BeginTable("##genome_legend", 4, ImGuiTableFlags_SizingStretchSame)) {
        auto cell = [](const char* label, ImVec4 color) {
            ImGui::TableNextColumn();
            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::TextUnformatted("\xE2\x96\xA0"); // ■
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::TextDisabled("%s", label);
        };
        cell("bit is 0", col::mantle);
        cell("flipped 1->0", col::red);
        cell("flipped 0->1", col::green);
        cell("bit is 1", col::teal);
        ImGui::EndTable();
    }

    ImPlotFlags hflags = ImPlotFlags_CanvasOnly | ImPlotFlags_NoLegend;
    if (!ImPlot::BeginPlot("##genome", ImVec2(-1, h), hflags))
        return;

    ImPlot::SetupAxis(ImAxis_X1, nullptr, ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxis(ImAxis_Y1, nullptr, ImPlotAxisFlags_NoDecorations);
    ImPlot::SetupAxisLimits(ImAxis_X1, 0, 1, ImPlotCond_Always);
    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1, ImPlotCond_Always);

    if (!heatmapData_.empty() && ga_) {
        int n = (int)sortedRows_.size();
        int l = ga_->problem().bitLength;
        // qual colormap + scale [0, 4] means values 0..3 each pick exactly one palette slot
        ImPlot::PushColormap(genomeCmap_);
        ImPlot::PlotHeatmap("##bits", heatmapData_.data(), n, l, 0.0, 4.0, nullptr);
        ImPlot::PopColormap();

        // hover tooltip: show which chromosome the row maps to + its fitness and decoded x
        if (ImPlot::IsPlotHovered()) {
            ImPlotPoint mp = ImPlot::GetPlotMousePos();
            // y axis runs [0,1] top->bottom inverted inside PlotHeatmap; row 0 is top
            int row = (int)std::floor((1.0 - mp.y) * n);
            int col = (int)std::floor(mp.x * l);
            if (row >= 0 && row < n && col >= 0 && col < l) {
                int idx = sortedRows_[row];
                const auto& c = lastTrace_.afterMutation[idx];
                ImGui::BeginTooltip();
                ImGui::Text("row %d  (chromosome #%d)", row, idx);
                ImGui::Text("x = %.6f   f(x) = %.6f", c.x, c.fitness);
                ImGui::Text("bit %d (MSB-first): %d", col, (int)c.bits[col] ? 1 : 0);
                double v = heatmapData_[(size_t)(row * l + col)];
                const char* status = v == kBitFlipped0to1   ? "flipped 0 -> 1"
                                     : v == kBitFlipped1to0 ? "flipped 1 -> 0"
                                     : v == kBitUnchanged1  ? "unchanged (1)"
                                                            : "unchanged (0)";
                ImGui::TextDisabled("%s", status);
                ImGui::EndTooltip();
            }
        }
    }

    ImPlot::EndPlot();
}

void GuiApp::drawToolbar_(float h) {
    ImGui::BeginChild("##toolbar", ImVec2(-1, h), false, ImGuiWindowFlags_NoScrollbar);

    bool canStep = ga_ && errorMsg_.empty() && currentGen_() < maxGen_();

    if (playing_) {
        if (ImGui::Button("Pause"))
            playing_ = false;
    } else {
        ImGui::BeginDisabled(!canStep);
        if (ImGui::Button("Play "))
            playing_ = true;
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    ImGui::BeginDisabled(!canStep);
    if (ImGui::Button("Step")) {
        playing_ = false;
        doStep_();
    }
    ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        editParams_ = baselineParams_;
        rebuild_(baselineParams_);
    }

    ImGui::SameLine();
    if (ImGui::Button("Fit view")) {
        scatterResetFrames_ = 2;
        convResetFrames_ = 2;
    }

    ImGui::SameLine();
    ImGui::Checkbox("follow", &autoFollow_);
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("while playing, keep the current generation near the right edge\n"
                          "of the convergence plot");

    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    ImGui::SliderFloat("gens/s", &gensPerSec_, 0.5f, 200.0f, "%.1f", ImGuiSliderFlags_Logarithmic);

    ImGui::SameLine();
    ImGui::Text(" gen %d / %d", currentGen_(), maxGen_());

    if (bestEverFitness_ > -std::numeric_limits<double>::infinity()) {
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, col::yellow);
        ImGui::Text("   best-ever:  x = %.6f   f = %.6f", bestEverX_, bestEverFitness_);
        ImGui::PopStyleColor();
    }

    ImGui::EndChild();
}

// main frame loop

void GuiApp::handleShortcuts_() {
    // only fire shortcuts when no text input has focus
    if (ImGui::GetIO().WantTextInput)
        return;

    bool canStep = ga_ && errorMsg_.empty() && currentGen_() < maxGen_();

    if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
        if (playing_)
            playing_ = false;
        else if (canStep)
            playing_ = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, false) && canStep) {
        playing_ = false;
        doStep_();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
        editParams_ = baselineParams_;
        rebuild_(baselineParams_);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
        randomizeCoefficients_();
    }
}

void GuiApp::frame() {
    handleShortcuts_();

    // advance the GA when playing
    if (playing_ && ga_ && currentGen_() < maxGen_()) {
        double dt = (double)ImGui::GetIO().DeltaTime;
        accumulator_ += dt * gensPerSec_;
        while (accumulator_ >= 1.0 && currentGen_() < maxGen_()) {
            accumulator_ -= 1.0;
            doStep_();
        }
    }
    if (playing_ && currentGen_() >= maxGen_())
        playing_ = false;

    // root window covering the entire viewport
    auto* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("##root", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoScrollWithMouse);

    const float toolbarH = 34.0f;

    // left: parameter panel
    ImGui::BeginChild("##left", ImVec2(kLeftPanelW, 0), true);
    drawParamsPanel_();
    ImGui::EndChild();

    ImGui::SameLine();

    // right: three stacked plots + toolbar strip
    ImGui::BeginChild("##right", ImVec2(0, 0), false);
    {
        float avail = ImGui::GetContentRegionAvail().y;
        float gap = ImGui::GetStyle().ItemSpacing.y;
        float legendH = ImGui::GetTextLineHeight() + 2.0f * ImGui::GetStyle().CellPadding.y;
        float plotH = (avail - toolbarH - 4.0f * gap - legendH) / 3.0f;

        drawScatterPanel_(plotH);
        drawHistoryPanel_(plotH);
        drawHeatmapPanel_(plotH);
        drawToolbar_(toolbarH);
    }
    ImGui::EndChild();

    ImGui::End();
}

// public entry point

int runGui(const GaParams& initialParams, uint64_t seed) {
    Platform platform("GA Visualizer", 1280, 800);
    applyTheme();
    GuiApp app(initialParams, seed);

    while (!platform.shouldClose()) {
        platform.beginFrame();
        app.frame();
        platform.endFrame();
    }
    return 0;
}

} // namespace gui

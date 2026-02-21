/* -- C++ -- */
/**
 *  @file  core/include/AnalysisModel.hh
 *
 *  @brief Declarative base model for analysis variables, selections,
 *         histograms, and snapshots.
 */

#ifndef HERON_CORE_ANALYSIS_MODEL_H
#define HERON_CORE_ANALYSIS_MODEL_H

#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "AnalysisContext.hh"

namespace heron
{

namespace detail
{

inline std::vector<std::string> merge_dependencies(const std::vector<std::string> &lhs, const std::vector<std::string> &rhs)
{
    std::vector<std::string> merged = lhs;
    std::set<std::string> seen(lhs.begin(), lhs.end());
    for (const std::string &dep : rhs)
    {
        if (seen.insert(dep).second)
            merged.push_back(dep);
    }

    return merged;
}

inline std::string combine_expression(const std::string &lhs, const std::string &rhs, const std::string &op)
{
    if (lhs.empty())
        return rhs;
    if (rhs.empty())
        return lhs;
    return "(" + lhs + ") " + op + " (" + rhs + ")";
}

} // namespace detail

template <typename TValue>
struct Column
{
    std::string expression;
    std::vector<std::string> dependencies;
};

template <typename TValue>
inline Column<TValue> col(std::string name)
{
    Column<TValue> c;
    c.expression = name;
    c.dependencies.push_back(std::move(name));
    return c;
}

template <typename TValue>
inline Column<TValue> expr(std::string expression)
{
    Column<TValue> c;
    c.expression = std::move(expression);
    return c;
}

struct CutExpression
{
    std::string expression;
    std::vector<std::string> dependencies;
};

/**
 *  \brief Named variable declaration with optional dependency names.
 */
template <typename TValue>
struct Var
{
    std::string name;
    std::function<TValue()> expression;
    std::vector<std::string> dependencies;
};

/**
 *  \brief Named cut predicate with optional dependency names.
 */
struct Cut
{
    std::string name;
    std::function<bool()> predicate;
    std::vector<std::string> dependencies;
    std::string expression;
};

/**
 *  \brief Named weight expression with optional dependency names.
 */
struct Weight
{
    std::string name;
    std::function<double()> expression;
    std::vector<std::string> dependencies;
    std::string column_expression;

    bool enabled = false;
};

/**
 *  \brief Named selection composed from one cut and an optional weight.
 */
struct Selection
{
    std::string name;
    Cut cut;
    Weight weight;
    std::string cut_expression;
};

inline CutExpression to_cut_expression(const Cut &c)
{
    CutExpression ce;
    ce.expression = c.expression.empty() ? c.name : c.expression;
    ce.dependencies = c.dependencies;
    return ce;
}

inline CutExpression operator&&(const CutExpression &lhs, const CutExpression &rhs)
{
    CutExpression ce;
    ce.expression = detail::combine_expression(lhs.expression, rhs.expression, "&&");
    ce.dependencies = detail::merge_dependencies(lhs.dependencies, rhs.dependencies);
    return ce;
}

inline CutExpression operator&&(const Cut &lhs, const Cut &rhs)
{
    return to_cut_expression(lhs) && to_cut_expression(rhs);
}

inline CutExpression operator&&(const CutExpression &lhs, const Cut &rhs)
{
    return lhs && to_cut_expression(rhs);
}

inline CutExpression operator&&(const Cut &lhs, const CutExpression &rhs)
{
    return to_cut_expression(lhs) && rhs;
}

/**
 *  \brief Named analysis channel declaration.
 */
struct Channel
{
    std::string name;
    std::function<int()> classifier;
    std::vector<std::string> dependencies;
};

/**
 *  \brief Descriptor for a one-dimensional histogram declaration.
 */
struct Hist1DSpec
{
    std::string name;
    std::string title;
    std::string variable;
    int bins = 0;
    double x_min = 0.0;
    double x_max = 0.0;
    std::string selection;
    std::string weight;
};

/**
 *  \brief Descriptor for a snapshot declaration.
 */
struct SnapshotSpec
{
    std::string name;
    std::vector<std::string> columns;
    std::string selection;
};

/**
 *  \brief Base class for declarative analysis models.
 *
 *  Typical usage in a concrete model:
 *
 *  \code
 *  class MuonSelectionModel : public heron::AnalysisModel
 *  {
 *    public:
 *      void define() override
 *      {
 *          const auto p_muon_p = var("muon_p", []() { return 0.0; }, {"reco_muon_p"});
 *          const auto c_fid = cut("fiducial", []() { return true; }, {"reco_vertex_x", "reco_vertex_y", "reco_vertex_z"});
 *          const auto w_cv = weight("cv", []() { return 1.0; }, {"event_weight_cv"});
 *          const auto s_nominal = selection("nominal", c_fid, w_cv);
 *
 *          hist1d("h_muon_p", p_muon_p.name, 40, 0.0, 2.0, "Muon momentum;p [GeV];Events", s_nominal.name, w_cv.name);
 *          snapshot("selected_events", {"run", "subrun", "event", p_muon_p.name}, s_nominal.name);
 *      }
 *  };
 *  \endcode
 */
class AnalysisModel
{
  public:
    virtual ~AnalysisModel() = default;

    /// Build the model from channel, selection, service, and output hooks.
    virtual void define()
    {
        define_channels();
        define_selections();
        define_services();
        define_outputs();
    }

    /// Declare analysis channels.
    virtual void define_channels() {}

    /// Declare analysis selections.
    virtual void define_selections() {}

    /// Define analysis-library service behaviour.
    virtual void define_services()
    {
        define_analysis_channels_service();
        define_analysis_config_service();
        define_column_derivation_service();
        define_event_sample_filter_service();
        define_rdataframe_service();
        define_selection_service();
    }

    /// Declare model variables, histograms, and snapshots.
    virtual void define_outputs() {}

    /// Override AnalysisChannels service behaviour.
    virtual void define_analysis_channels_service() {}

    /// Override AnalysisConfigService behaviour.
    virtual void define_analysis_config_service() {}

    /// Override ColumnDerivationService behaviour.
    virtual void define_column_derivation_service() {}

    /// Override EventSampleFilterService behaviour.
    virtual void define_event_sample_filter_service() {}

    /// Override RDataFrameService behaviour.
    virtual void define_rdataframe_service() {}

    /// Override SelectionService behaviour.
    virtual void define_selection_service() {}

    /// Optional hook for external configuration/services.
    virtual void configure() {}

    /// Optional context-aware configuration hook.
    template <typename TPolicy, typename TServices>
    void configure(const AnalysisContext<TPolicy, TServices> &context)
    {
        (void) context;
        configure();
    }

    /// Reset and build the model using default configuration.
    bool initialise()
    {
        clear();
        configure();
        define();
        return true;
    }

    /// Reset and build the model using an analysis context.
    template <typename TPolicy, typename TServices>
    bool initialise(const AnalysisContext<TPolicy, TServices> &context)
    {
        clear();
        configure(context);
        define();
        return true;
    }

    const std::vector<Var<double> > &vars() const noexcept { return m_vars; }
    const std::vector<Channel> &channels() const noexcept { return m_channels; }
    const std::vector<Cut> &cuts() const noexcept { return m_cuts; }
    const std::vector<Weight> &weights() const noexcept { return m_weights; }
    const std::vector<Selection> &selections() const noexcept { return m_selections; }
    const std::vector<Hist1DSpec> &h1() const noexcept { return m_h1; }
    const std::vector<SnapshotSpec> &snapshots() const noexcept { return m_snapshots; }

  protected:
    template <typename TClassifier>
    Channel channel(std::string name, TClassifier classifier, std::vector<std::string> dependencies = {})
    {
        Channel c;
        c.name = std::move(name);
        c.classifier = std::function<int()>(std::move(classifier));
        c.dependencies = std::move(dependencies);
        m_channels.push_back(c);
        return c;
    }

    template <typename TExpression>
    Var<double> var(std::string name, TExpression expression, std::vector<std::string> dependencies = {})
    {
        Var<double> v;
        v.name = std::move(name);
        v.expression = std::function<double()>(std::move(expression));
        v.dependencies = std::move(dependencies);
        m_vars.push_back(v);
        return v;
    }

    template <typename TPredicate>
    Cut cut(std::string name, TPredicate predicate, std::vector<std::string> dependencies = {})
    {
        Cut c;
        c.name = std::move(name);
        c.predicate = std::function<bool()>(std::move(predicate));
        c.dependencies = std::move(dependencies);
        c.expression = c.name;
        m_cuts.push_back(c);
        return c;
    }

    Cut cut(std::string name, const Column<bool> &column)
    {
        Cut c;
        c.name = std::move(name);
        c.predicate = std::function<bool()>();
        c.dependencies = column.dependencies;
        c.expression = column.expression;
        m_cuts.push_back(c);
        return c;
    }

    template <typename TExpression>
    Weight weight(std::string name, TExpression expression, std::vector<std::string> dependencies = {})
    {
        Weight w;
        w.name = std::move(name);
        w.expression = std::function<double()>(std::move(expression));
        w.dependencies = std::move(dependencies);
        w.column_expression = w.name;
        w.enabled = true;
        m_weights.push_back(w);
        return w;
    }

    Weight weight(std::string name, const Column<double> &column)
    {
        Weight w;
        w.name = std::move(name);
        w.expression = std::function<double()>();
        w.dependencies = column.dependencies;
        w.column_expression = column.expression;
        w.enabled = true;
        m_weights.push_back(w);
        return w;
    }

    Selection selection(std::string name, Cut c, Weight w = {});
    Selection selection(std::string name, const CutExpression &c, Weight w = {});
    Hist1DSpec hist1d(std::string name,
                      std::string variable,
                      int bins,
                      double x_min,
                      double x_max,
                      std::string title = "",
                      std::string selection = "",
                      std::string weight = "");
    SnapshotSpec snapshot(std::string name, std::vector<std::string> columns, std::string selection = "");
    void clear() noexcept;

  private:
    std::vector<Var<double> > m_vars;
    std::vector<Channel> m_channels;
    std::vector<Cut> m_cuts;
    std::vector<Weight> m_weights;
    std::vector<Selection> m_selections;
    std::vector<Hist1DSpec> m_h1;
    std::vector<SnapshotSpec> m_snapshots;
};

} // namespace heron

#endif // HERON_CORE_ANALYSIS_MODEL_H

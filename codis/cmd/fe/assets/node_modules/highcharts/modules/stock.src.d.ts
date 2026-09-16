/*!*
 *
 *  Copyright (c) Highsoft AS. All rights reserved.
 *
 *!*/
import * as globals from "../globals.src";
import * as _Highcharts from "../highcharts.src";
/**
 * Adds the module to the imported Highcharts namespace.
 *
 * @param highcharts
 *        The imported Highcharts namespace to extend.
 */
export function factory(highcharts: typeof Highcharts): void;
declare module "../highcharts.src" {
    interface Axis {
        /**
         * (Highstock) Check the groupPixelWidth and apply the grouping if
         * needed. Fired only after processing the data.
         */
        applyGrouping(): void;
        /**
         * Highcharts Stock only. Set the compare mode on all series belonging
         * to a Y axis.
         *
         * @param compare
         *        The compare mode. Can be one of `undefined` (default), `null`,
         *        `"value"` or `"percent"`.
         *
         * @param redraw
         *        Whether to redraw the chart or to wait for a later call to
         *        Chart#redraw.
         */
        setCompare(compare?: (string|null), redraw?: boolean): void;
        /**
         * Highcharts Stock only. Set the cumulative mode on all series
         * belonging to a Y axis.
         *
         * @param cumulative
         *        Whether to disable or enable the cumulative mode. Can be one
         *        of `undefined` (default, treated as `false`), `false` or
         *        `true`.
         *
         * @param redraw
         *        Whether to redraw the chart or to wait for a later call to
         *        Chart#redraw.
         */
        setCumulative(cumulative?: boolean, redraw?: boolean): void;
        /**
         * (Highstock) Highcharts Stock only. Force data grouping on all the
         * axis' series.
         *
         * @param dataGrouping
         *        A `dataGrouping` configuration. Use `false` to disable data
         *        grouping dynamically.
         *
         * @param redraw
         *        Whether to redraw the chart or wait for a later call to
         *        Chart#redraw.
         */
        setDataGrouping(dataGrouping?: (boolean|DataGroupingOptionsObject), redraw?: boolean): void;
    }
    interface Series {
        /**
         * Highcharts Stock only. Set the compare mode of the series after
         * render time. In most cases it is more useful running Axis#setCompare
         * on the X axis to update all its series.
         *
         * @param compare
         *        Can be one of `undefined` (default), `null`, `"percent"` or
         *        `"value"`.
         *
         * @param redraw
         *        Whether to redraw the chart or to wait for a later call to
         *        Chart#redraw.
         */
        setCompare(compare?: (string|null), redraw?: boolean): void;
        /**
         * Highcharts Stock only. Set the cumulative mode of the series after
         * render time. In most cases it is more useful running
         * Axis#setCumulative on the Y axis to update all its series.
         *
         * @param cumulative
         *        Either enable or disable Cumulative Sum mode. Can be one of
         *        `false` (default) or `true`.
         *
         * @param redraw
         *        Whether to redraw the chart or to wait for a later call to
         *        Chart#redraw.
         */
        setCumulative(cumulative?: boolean, redraw?: boolean): void;
    }
    /**
     * Stock-optimized chart. Use Chart for common charts.
     */
    class StockChart extends Chart {
        /**
         * Initializes the chart. The constructor's arguments are passed on
         * directly.
         *
         * @param userOptions
         *        Custom options.
         *
         * @param callback
         *        Function to run when the chart has loaded and and all external
         *        images are loaded.
         *
         * @fires Highcharts.StockChart#init
         * @fires Highcharts.StockChart#afterInit
         */
        init(userOptions: Options, callback?: Function): void;
    }
    /**
     * Factory function for creating new stock charts. Creates a new StockChart
     * object with different default options than the basic Chart.
     *
     * @param options
     *        The chart options structure as described in the options reference.
     *
     * @param callback
     *        A function to execute when the chart object is finished loading
     *        and rendering. In most cases the chart is built in one thread, but
     *        in Internet Explorer version 8 or less the chart is sometimes
     *        initialized before the document is ready, and in these cases the
     *        chart object will not be finished synchronously. As a consequence,
     *        code that relies on the newly built Chart object should always run
     *        in the callback. Defining a chart.events.load handler is
     *        equivalent.
     *
     * @return The chart object.
     */
    function stockChart(options: Options, callback?: ChartCallbackFunction): StockChart;
    /**
     * Factory function for creating new stock charts. Creates a new StockChart
     * object with different default options than the basic Chart.
     *
     * @param renderTo
     *        The DOM element to render to, or its id.
     *
     * @param options
     *        The chart options structure as described in the options reference.
     *
     * @param callback
     *        A function to execute when the chart object is finished loading
     *        and rendering. In most cases the chart is built in one thread, but
     *        in Internet Explorer version 8 or less the chart is sometimes
     *        initialized before the document is ready, and in these cases the
     *        chart object will not be finished synchronously. As a consequence,
     *        code that relies on the newly built Chart object should always run
     *        in the callback. Defining a chart.events.load handler is
     *        equivalent.
     *
     * @return The chart object.
     */
    function stockChart(renderTo: (string|HTMLDOMElement), options: Options, callback?: ChartCallbackFunction): StockChart;
    /**
     * When we have vertical scrollbar, rifles and arrow in buttons should be
     * rotated. The same method is used in Navigator's handles, to rotate them.
     *
     * @param path
     *        Path to be rotated.
     *
     * @param vertical
     *        If vertical scrollbar, swap x-y values.
     *
     * @return Rotated path.
     */
    function swapXY(path: SVGPathArray, vertical?: boolean): SVGPathArray;
    /**
     * The symbol callbacks are generated on the SVGRenderer object in all
     * browsers. Even VML browsers need this in order to generate shapes in
     * export. Now share them with the VMLRenderer.
     */
    let Renderer: any;
}
export default factory;
export let Highcharts: typeof _Highcharts;

/*!*
 *
 *  Copyright (c) Highsoft AS. All rights reserved.
 *
 *!*/
import * as globals from "./globals.src";
import * as _Highcharts from "./highcharts.src";
/**
 * Adds the module to the imported Highcharts namespace.
 *
 * @param highcharts
 *        The imported Highcharts namespace to extend.
 */
export function factory(highcharts: typeof Highcharts): void;
declare module "./highcharts.src" {
    /**
     * Merge the default options with custom options and return the new options
     * structure. Commonly used for defining reusable templates.
     *
     * @param options
     *        The new custom chart options.
     */
    function setOptions(options: Options): void;
    /**
     * If ranges are not specified, determine ranges from rendered bubble series
     * and render legend again.
     */
    function chartDrawChartBox(): void;
    /**
     * Wrap the getOffset method to return zero offset for title or labels in a
     * radial axis.
     */
    function getOffset(): void;
    /**
     * Find the path for plot lines perpendicular to the radial axis.
     */
    function getPlotLinePath(): void;
    /**
     * Find the position for the axis title, by default inside the gauge.
     */
    function getTitlePosition(): void;
    /**
     * Finalize modification of axis instance with radial logic.
     */
    function onAxisAfterInit(): void;
    /**
     * Wrap auto label align to avoid setting axis-wide rotation on radial axes.
     * (#4920)
     */
    function onAxisAutoLabelAlign(): void;
    /**
     * Remove label collector function on axis remove/update.
     */
    function onAxisDestroy(): void;
    /**
     * Modify axis instance with radial logic before common axis init.
     */
    function onAxisInit(): void;
    /**
     * Prepare axis translation.
     */
    function onAxisInitialAxisTranslation(): void;
    /**
     * Start the bubble legend creation process.
     */
    function onLegendAfterGetAllItems(): void;
    /**
     * Toggle bubble legend depending on the visible status of bubble series.
     */
    function onSeriesLegendItemClick(): void;
    /**
     * Find the center position of the label based on the distance option.
     */
    function onTickAfterGetLabelPosition(): void;
    /**
     * Add special cases within the Tick class' methods for radial axes.
     */
    function onTickAfterGetPosition(): void;
    /**
     * Prevent setting Y axis dirty.
     */
    function renderHidden(): void;
    /**
     * Wrap the getMarkPath function to return the path of the radial marker.
     */
    function wrapTickGetMarkPath(): void;
}
export default factory;
export let Highcharts: typeof _Highcharts;

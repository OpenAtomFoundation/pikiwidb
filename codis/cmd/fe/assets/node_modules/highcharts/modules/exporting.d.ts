/*!*
 *
 *  Copyright (c) Highsoft AS. All rights reserved.
 *
 *!*/
import * as globals from "../globals";
import * as _Highcharts from "../highcharts";
/**
 * Adds the module to the imported Highcharts namespace.
 *
 * @param highcharts
 *        The imported Highcharts namespace to extend.
 */
export function factory(highcharts: typeof Highcharts): void;
declare module "../highcharts" {
    interface AjaxSettingsObject {
        data: (string|Dictionary<any>);
        dataType: ("json"|"octet"|"text"|"xml");
        error: Function;
        headers: Dictionary<string>;
        success: Function;
        type: string;
        url: string;
    }
    interface Chart {
        fullscreen: Fullscreen;
        /**
         * Exporting module required. Submit an SVG version of the chart to a
         * server along with some parameters for conversion.
         *
         * @param exportingOptions
         *        Exporting options in addition to those defined in exporting.
         *
         * @param chartOptions
         *        Additional chart options for the exported chart. For example a
         *        different background color can be added here, or `dataLabels`
         *        for export only.
         */
        exportChart(exportingOptions: ExportingOptions, chartOptions: Options): void;
        /**
         * Return the unfiltered innerHTML of the chart container. Used as hook
         * for plugins. In styled mode, it also takes care of inlining CSS style
         * rules.
         *
         * @return The unfiltered SVG of the chart.
         */
        getChartHTML(): string;
        /**
         * Get the default file name used for exported charts. By default it
         * creates a file name based on the chart title.
         *
         * @return A file name without extension.
         */
        getFilename(): string;
        /**
         * Return an SVG representation of the chart.
         *
         * @param chartOptions
         *        Additional chart options for the generated SVG representation.
         *        For collections like `xAxis`, `yAxis` or `series`, the
         *        additional options is either merged in to the original item of
         *        the same `id`, or to the first item if a common id is not
         *        found.
         *
         * @return The SVG representation of the rendered chart.
         *
         * @fires Highcharts.Chart#getSVG
         */
        getSVG(chartOptions?: Options): string;
        /**
         * Exporting module required. Clears away other elements in the page and
         * prints the chart as it is displayed. By default, when the exporting
         * module is enabled, a context button with a drop down menu in the
         * upper right corner accesses this function.
         *
         * @fires Highcharts.Chart#beforePrint
         * @fires Highcharts.Chart#afterPrint
         */
        print(): void;
    }
    /**
     * Handles displaying chart's container in the fullscreen mode.
     *
     * **Note**: Fullscreen is not supported on iPhone due to iOS limitations.
     */
    class Fullscreen {
        /**
         * Chart managed by the fullscreen controller.
         */
        chart: Chart;
        /**
         * The flag is set to `true` when the chart is displayed in the
         * fullscreen mode.
         */
        isOpen?: boolean;
        /**
         * Stops displaying the chart in fullscreen mode. Exporting module
         * required.
         */
        close(): void;
        /**
         * Displays the chart in fullscreen mode. When fired customly by user
         * before exporting context button is created, button's text will not be
         * replaced - it's on the user side. Exporting module required.
         */
        open(): void;
        /**
         * Toggles displaying the chart in fullscreen mode. By default, when the
         * exporting module is enabled, a context button with a drop down menu
         * in the upper right corner accesses this function. Exporting module
         * required.
         */
        toggle(): void;
    }
    /**
     * Perform an Ajax call.
     *
     * @param attr
     *        The Ajax settings to use.
     *
     * @return Returns false, if error occured.
     */
    function ajax(attr: Partial<AjaxSettingsObject>): (false|undefined);
    /**
     * Get a JSON resource over XHR, also supporting CORS without preflight.
     *
     * @param url
     *        The URL to load.
     *
     * @param success
     *        The success callback. For error handling, use the
     *        `Highcharts.ajax` function instead.
     */
    function getJSON(url: string, success: Function): void;
}
export default factory;
export let Highcharts: typeof _Highcharts;

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
    interface Chart {
        /**
         * Returns true if the chart is in series boost mode.
         *
         * @param chart
         *        the chart to check
         *
         * @return true if the chart is in series boost mode
         */
        isChartSeriesBoosting(chart: Chart): boolean;
    }
    interface Series {
        /**
         * If implemented in the core, parts of this can probably be shared with
         * other similar methods in Highcharts.
         */
        destroyGraphics(): void;
        /**
         * Enter boost mode and apply boost-specific properties.
         */
        enterBoost(): void;
        /**
         * Exit from boost mode and restore non-boost properties.
         */
        exitBoost(): void;
        /**
         * Return a full Point object based on the index. The boost module uses
         * stripped point objects for performance reasons.
         *
         * @param boostPoint
         *        A stripped-down point object
         *
         * @return A Point object as per
         *         https://api.highcharts.com/highcharts#Point
         */
        getPoint(boostPoint: (object|Point)): Point;
    }
    /**
     * Initialize the canvas boost.
     */
    function initCanvasBoost(): void;
}
export default factory;
export let Highcharts: typeof _Highcharts;

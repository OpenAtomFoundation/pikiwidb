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
         * Drill down to a given category. This is the same as clicking on an
         * axis label. If multiple series with drilldown are present, all will
         * drill down to the given category.
         *
         * See also `Point.doDrilldown` for drilling down on a single point
         * instance.
         *
         * @param x
         *        The index of the category
         *
         * @param originalEvent
         *        The original event, used internally.
         */
        drilldownCategory(x: number, originalEvent?: MouseEvent): void;
    }
    interface Chart {
        /**
         * Add a series to the chart as drilldown from a specific point in the
         * parent series. This method is used for async drilldown, when clicking
         * a point in a series should result in loading and displaying a more
         * high-resolution series. When not async, the setup is simpler using
         * the drilldown.series options structure.
         *
         * @param point
         *        The point from which the drilldown will start.
         *
         * @param options
         *        The series options for the new, detailed series.
         */
        addSeriesAsDrilldown(point: Point, options: SeriesOptionsType): void;
        /**
         * When the chart is drilled down to a child series, calling
         * `chart.drillUp()` will drill up to the parent series.
         */
        drillUp(): void;
    }
    interface Point {
        /**
         * Perform drilldown on a point instance. The drilldown property must be
         * set on the point options.
         *
         * To drill down multiple points in the same category, use
         * `Axis.drilldownCategory` instead.
         */
        doDrilldown(): void;
    }
    interface SVGElement {
        /**
         * A general fadeIn method.
         *
         * @param animation
         *        The animation options for the element fade.
         */
        fadeIn(animation?: (boolean|Partial<AnimationOptionsObject>)): void;
    }
}
export default factory;
export let Highcharts: typeof _Highcharts;

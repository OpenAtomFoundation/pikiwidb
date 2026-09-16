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
         * Add an annotation to the chart after render time.
         *
         * @param options
         *        The annotation options for the new, detailed annotation.
         *
         * @return - The newly generated annotation.
         */
        addAnnotation(options: AnnotationsOptions, redraw?: boolean): Annotation;
        /**
         * Remove an annotation from the chart.
         *
         * @param idOrAnnotation
         *        The annotation's id or direct annotation object.
         */
        removeAnnotation(idOrAnnotation: (number|string|Annotation)): void;
    }
    interface Point {
        mock?: boolean;
    }
    /**
     * Function which generates the path of the halfcircle.
     *
     * @param pixelInterval
     *        diameter of the circle in pixels
     *
     * @param numberOfCircles
     *        number of cricles
     *
     * @param startX
     *        x position of the first circle
     *
     * @param y
     *        y position of the bottom of the timeCycles
     *
     * @return path
     */
    function getCirclePath(pixelInterval: number, numberOfCircles: number, startX: number, y: number): string;
    /**
     * Function to create start of the path.
     *
     * @param x
     *        x position of the TimeCycles
     *
     * @param y
     *        y position of the TimeCycles
     *
     * @return path
     */
    function getStartingPath(x: number, y: number): string;
}
export default factory;
export let Highcharts: typeof _Highcharts;

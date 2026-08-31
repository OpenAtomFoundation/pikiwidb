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
    interface Chart {
        /**
         * Highmaps only. Get chart coordinates from latitude/longitude. Returns
         * an object with x and y values corresponding to the `xAxis` and
         * `yAxis`.
         *
         * @param latLon
         *        Coordinates.
         *
         * @return X and Y coordinates in terms of chart axis values.
         */
        fromLatLonToPoint(latLon: MapLatLonObject): MapCoordinateObject;
        /**
         * Highmaps only. Calculate latitude/longitude values for a point.
         * Returns an object with the numeric properties `lat` and `lon`.
         *
         * @param point
         *        A `Point` instance or anything containing `x` and `y`
         *        properties with numeric values.
         *
         * @return An object with `lat` and `lon` properties.
         */
        fromPointToLatLon(point: (MapCoordinateObject|Point)): (MapLatLonObject|undefined);
        /**
         * Highcharts Maps only. Zoom in or out of the map. See also
         * Point#zoomTo. See Chart#fromLatLonToPoint for how to get the
         * `centerX` and `centerY` parameters for a geographic location.
         *
         * Deprecated as of v9.3 in favor of MapView.zoomBy.
         *
         * @param howMuch
         *        How much to zoom the map. Values less than 1 zooms in. 0.5
         *        zooms in to half the current view. 2 zooms to twice the
         *        current view. If omitted, the zoom is reset.
         *
         * @param xProjected
         *        The projected x position to keep stationary when zooming, if
         *        available space.
         *
         * @param yProjected
         *        The projected y position to keep stationary when zooming, if
         *        available space.
         *
         * @param chartX
         *        Keep this chart position stationary if possible. This is used
         *        for example in mousewheel events, where the area under the
         *        mouse should be fixed as we zoom in.
         *
         * @param chartY
         *        Keep this chart position stationary if possible.
         */
        mapZoom(howMuch?: number, xProjected?: number, yProjected?: number, chartX?: number, chartY?: number): void;
        /**
         * Highmaps only. Get point from latitude and longitude using specified
         * transform definition.
         *
         * @param latLon
         *        A latitude/longitude object.
         *
         * @param transform
         *        The transform definition to use as explained in the
         *        documentation.
         *
         * @return An object with `x` and `y` properties.
         */
        transformFromLatLon(latLon: MapLatLonObject, transform: any): MapCoordinateObject;
        /**
         * Highmaps only. Get latLon from point using specified transform
         * definition. The method returns an object with the numeric properties
         * `lat` and `lon`.
         *
         * @param point
         *        A `Point` instance, or any object containing the properties
         *        `x` and `y` with numeric values.
         *
         * @param transform
         *        The transform definition to use as explained in the
         *        documentation.
         *
         * @return An object with `lat` and `lon` properties.
         */
        transformToLatLon(point: (MapCoordinateObject|Point), transform: any): (MapLatLonObject|undefined);
    }
    /**
     * Result object of a map transformation.
     */
    interface MapCoordinateObject {
        /**
         * X coordinate on the map.
         */
        x: number;
        /**
         * Y coordinate on the map.
         */
        y: (number|null);
    }
    /**
     * A latitude/longitude object.
     */
    interface MapLatLonObject {
        /**
         * The latitude.
         */
        lat: number;
        /**
         * The longitude.
         */
        lon: number;
    }
    interface Point {
        pointPadding?: number;
        /**
         * In Highmaps, when data is loaded from GeoJSON, the GeoJSON item's
         * properies are copied over here.
         */
        properties: any;
        value?: (number|null);
        /**
         * Highmaps only. Zoom in on the point using the global animation.
         */
        zoomTo(): void;
    }
    interface PointOptionsObject {
        pointPadding?: number;
        value?: (number|null);
    }
    /**
     * Map-optimized chart. Use Chart for common charts.
     */
    class MapChart extends Chart {
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
         * @fires Highcharts.MapChart#init
         * @fires Highcharts.MapChart#afterInit
         */
        init(userOptions: Options, callback?: Function): void;
    }
    /**
     * The map view handles zooming and centering on the map, and various
     * client-side projection capabilities.
     *
     * On a chart instance, the map view is available as `chart.mapView`.
     */
    class MapView {
        /**
         * The map view handles zooming and centering on the map, and various
         * client-side projection capabilities.
         *
         * On a chart instance, the map view is available as `chart.mapView`.
         *
         * @param chart
         *        The Chart instance
         *
         * @param options
         *        MapView options
         */
        constructor(chart: Chart, options: MapViewOptions);
        /**
         * The current center of the view in terms of `[longitude, latitude]`.
         */
        readonly center: LonLatArray;
        /**
         * The current zoom level of the view.
         */
        readonly zoom: number;
        /**
         * Fit the view to given bounds
         *
         * @param bounds
         *        Bounds in terms of projected units given as `{ x1, y1, x2, y2
         *        }`. If not set, fit to the bounds of the current data set
         *
         * @param padding
         *        Padding inside the bounds. A number signifies pixels, while a
         *        percentage string (like `5%`) can be used as a fraction of the
         *        plot area size.
         *
         * @param redraw
         *        Whether to redraw the chart immediately
         *
         * @param animation
         *        What animation to use for redraw
         */
        fitToBounds(bounds: object, padding?: (number|string), redraw?: boolean, animation?: (boolean|Partial<AnimationOptionsObject>)): void;
        /**
         * Convert pixel position to projected units
         *
         * @param pos
         *        The position in pixels
         *
         * @return The position in projected units
         */
        pixelsToProjectedUnits(pos: PositionObject): PositionObject;
        /**
         * Convert projected units to pixel position
         *
         * @param pos
         *        The position in projected units
         *
         * @return The position in pixels
         */
        projectedUnitsToPixels(pos: PositionObject): PositionObject;
        /**
         * Set the view to given center and zoom values.
         *
         * @param center
         *        The center point
         *
         * @param zoom
         *        The zoom level
         *
         * @param redraw
         *        Whether to redraw immediately
         *
         * @param animation
         *        Animation options for the redraw
         */
        setView(center: (LonLatArray|undefined), zoom: number, redraw?: boolean, animation?: (boolean|Partial<AnimationOptionsObject>)): void;
        /**
         * Update the view with given options
         *
         * @param options
         *        The new map view options to apply
         *
         * @param redraw
         *        Whether to redraw immediately
         *
         * @param animation
         *        The animation to apply to a the redraw
         */
        update(options: Partial<MapViewOptions>, redraw?: boolean, animation?: (boolean|Partial<AnimationOptionsObject>)): void;
        /**
         * Zoom the map view by a given number
         *
         * @param howMuch
         *        The amount of zoom to apply. 1 zooms in on half the current
         *        view, -1 zooms out. Pass `undefined` to zoom to the full
         *        bounds of the map.
         *
         * @param coords
         *        Optional map coordinates to keep fixed
         *
         * @param chartCoords
         *        Optional chart coordinates to keep fixed, in pixels
         *
         * @param animation
         *        The animation to apply to a the redraw
         */
        zoomBy(howMuch?: number, coords?: LonLatArray, chartCoords?: Array<number>, animation?: (boolean|Partial<AnimationOptionsObject>)): void;
    }
    /**
     * Contains all loaded map data for Highmaps.
     */
    let maps: Record<string, any>;
    /**
     * Highmaps only. Restructure a GeoJSON object in preparation to be read
     * directly by the series.mapData option. The GeoJSON will be broken down to
     * fit a specific Highcharts type, either `map`, `mapline` or `mappoint`.
     * Meta data in GeoJSON's properties object will be copied directly over to
     * Point.properties in Highmaps.
     *
     * @param geojson
     *        The GeoJSON structure to parse, represented as a JavaScript object
     *        rather than a JSON string.
     *
     * @param hType
     *        The Highmaps series type to prepare for. Setting "map" will return
     *        GeoJSON polygons and multipolygons. Setting "mapline" will return
     *        GeoJSON linestrings and multilinestrings. Setting "mappoint" will
     *        return GeoJSON points and multipoints.
     *
     * @return An object ready for the `mapData` option.
     */
    function geojson(geojson: GeoJSON, hType?: string): Array<any>;
    /**
     * The factory function for creating new map charts. Creates a new MapChart
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
    function mapChart(options: Options, callback?: ChartCallbackFunction): MapChart;
    /**
     * The factory function for creating new map charts. Creates a new MapChart
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
    function mapChart(renderTo: (string|HTMLDOMElement), options: Options, callback?: ChartCallbackFunction): MapChart;
    /**
     * Utility for reading SVG paths directly.
     *
     * @return Splitted SVG path
     */
    function splitPath(path: (string|Array<(string|number)>)): SVGPathArray;
    /**
     * The world size in terms of 10k meters in the Web Mercator projection, to
     * match a 256 square tile to zoom level 0
     */
    let worldSize: any;
    /**
     * If ranges are not specified, determine ranges from rendered bubble series
     * and render legend again.
     */
    function chartDrawChartBox(): void;
    /**
     * Toggle bubble legend depending on the visible status of bubble series.
     */
    function onSeriesLegendItemClick(): void;
}
export default factory;
export let Highcharts: typeof _Highcharts;

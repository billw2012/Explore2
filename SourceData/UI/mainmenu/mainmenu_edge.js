/**
 * Adobe Edge: symbol definitions
 */
(function($, Edge, compId){
//images folder
var im='images/';

var fonts = {};
var opts = {
    'gAudioPreloadPreference': 'auto',

    'gVideoPreloadPreference': 'auto'
};
var resources = [
];
var symbols = {
"stage": {
    version: "4.0.0",
    minimumCompatibleVersion: "4.0.0",
    build: "4.0.0.359",
    baseState: "Base State",
    scaleToFit: "none",
    centerStage: "both",
    initialState: "Base State",
    gpuAccelerate: false,
    resizeInstances: false,
    content: {
            dom: [
            {
                id: 'Rectangle2',
                type: 'rect',
                rect: ['-1000px', '-1000px','4000px','4000px','auto', 'auto'],
                fill: ["rgba(0,0,0,1.00)"],
                stroke: [0,"rgb(0, 0, 0)","none"]
            },
            {
                id: 'MenuItems',
                type: 'rect',
                rect: ['313px', '217px','894px','435px','auto', 'auto'],
                fill: ["rgba(50,50,50,0.00)"],
                stroke: [0,"rgb(0, 0, 0)","none"],
                filter: [0, 0, 1, 1, 0, 0, 0, 0, "rgba(0,0,0,0)", 0, 0, 0]
            }],
            symbolInstances: [

            ]
        },
    states: {
        "Base State": {
            "${_Stage}": [
                ["color", "background-color", 'rgba(0,0,0,1.00)'],
                ["style", "width", '1280px'],
                ["style", "height", '720px'],
                ["style", "overflow", 'visible']
            ],
            "${_Rectangle2}": [
                ["style", "top", '-1000px'],
                ["style", "height", '4000px'],
                ["color", "background-color", 'rgba(0,0,0,1.00)'],
                ["style", "left", '-1000px'],
                ["style", "width", '4000px']
            ],
            "${_MenuItems}": [
                ["color", "background-color", 'rgba(50,50,50,0.00)'],
                ["style", "top", '217px'],
                ["style", "height", '435px'],
                ["subproperty", "filter.blur", '0px'],
                ["style", "left", '313px'],
                ["style", "width", '894px']
            ]
        }
    },
    timelines: {
        "Default Timeline": {
            fromState: "Base State",
            toState: "",
            duration: 0,
            autoPlay: true,
            timeline: [
            ]
        }
    }
},
"MainMenuEntry": {
    version: "4.0.0",
    minimumCompatibleVersion: "4.0.0",
    build: "4.0.0.359",
    baseState: "Base State",
    scaleToFit: "none",
    centerStage: "none",
    initialState: "Base State",
    gpuAccelerate: false,
    resizeInstances: false,
    content: {
            dom: [
                {
                    rect: ['0px', '1px', '341px', '47px', 'auto', 'auto'],
                    filter: [0, 0, 1, 1, 0, 0, 0, 0, 'rgba(0,0,0,0)', 0, 0, 0],
                    id: 'Text',
                    text: 'Text',
                    font: ['Verdana, Geneva, sans-serif', 32, 'rgba(172,172,172,1.00)', '400', 'none', 'normal'],
                    type: 'text'
                },
                {
                    rect: ['379px', '14px', '407px', '23px', 'auto', 'auto'],
                    font: ['Verdana, Geneva, sans-serif', 16, 'rgba(255,255,255,1)', '400', 'none', 'normal'],
                    filter: [0, 0, 1, 1, 0, 0, 0, 0, 'rgba(0,0,0,0)', 0, 0, 0],
                    id: 'Description',
                    text: 'Description',
                    align: 'right',
                    type: 'text'
                },
                {
                    rect: ['0%', '0%', '100%', '100%', 'auto', 'auto'],
                    id: 'BackgroundRect',
                    stroke: [0, 'rgb(0, 0, 0)', 'none'],
                    type: 'rect',
                    fill: ['rgba(255,255,255,0.00)'],
                    c: [
                    {
                        rect: ['-1226px', 'auto', '800px', '1px', 'auto', '0px'],
                        id: 'BottomLine',
                        stroke: [0, 'rgba(0,0,0,1)', 'none'],
                        type: 'rect',
                        fill: ['rgba(255,255,255,1.00)']
                    }]
                },
                {
                    rect: ['-1017px', '-2px', '36px', '40px', 'auto', 'auto'],
                    id: 'HeightRect',
                    stroke: [0, 'rgb(0, 0, 0)', 'none'],
                    type: 'rect',
                    fill: ['rgba(255,255,255,0)']
                }
            ],
            symbolInstances: [
            ]
        },
    states: {
        "Base State": {
            "${_HeightRect}": [
                ["style", "height", '40px'],
                ["style", "left", '-1017px'],
                ["style", "top", '-2px']
            ],
            "${_BottomLine}": [
                ["style", "top", 'auto'],
                ["style", "bottom", '0px'],
                ["style", "height", '1px'],
                ["color", "background-color", 'rgba(255,255,255,1.00)'],
                ["style", "left", '-1226px']
            ],
            "${_Text}": [
                ["transform", "rotateZ", '0deg'],
                ["color", "color", 'rgba(172,172,172,0)'],
                ["style", "font-weight", '400'],
                ["style", "left", '563px'],
                ["style", "font-size", '24px'],
                ["style", "top", '2px'],
                ["style", "font-style", 'normal'],
                ["style", "height", '32px'],
                ["style", "font-family", 'Verdana, Geneva, sans-serif'],
                ["style", "text-decoration", 'none'],
                ["style", "width", '341px']
            ],
            "${symbolSelector}": [
                ["style", "height", '40px'],
                ["style", "width", '800px']
            ],
            "${_BackgroundRect}": [
                ["color", "background-color", 'rgba(255,255,255,0.00)'],
                ["style", "left", '0%']
            ],
            "${_Description}": [
                ["color", "color", 'rgba(184,184,184,0.00)'],
                ["style", "font-weight", '400'],
                ["style", "left", '383px'],
                ["style", "width", '407px'],
                ["style", "top", '7px'],
                ["style", "text-align", 'right'],
                ["style", "height", '23px'],
                ["style", "font-family", 'Verdana, Geneva, sans-serif'],
                ["style", "font-size", '16px']
            ]
        }
    },
    timelines: {
        "Default Timeline": {
            fromState: "Base State",
            toState: "",
            duration: 562,
            autoPlay: false,
            labels: {
                "Unselected": 500,
                "Selected": 562
            },
            timeline: [
                { id: "eid70", tween: [ "style", "${_HeightRect}", "height", '67px', { fromValue: '40px'}], position: 500, duration: 62 },
                { id: "eid30", tween: [ "color", "${_Description}", "color", 'rgba(255,255,255,1.00)', { animationColorSpace: 'RGB', valueTemplate: undefined, fromValue: 'rgba(184,184,184,0.00)'}], position: 500, duration: 62, easing: "easeOutQuad" },
                { id: "eid45", tween: [ "style", "${_BottomLine}", "left", '0px', { fromValue: '-1226px'}], position: 0, duration: 500, easing: "easeOutQuad" },
                { id: "eid15", tween: [ "color", "${_Text}", "color", 'rgba(134,134,134,1.00)', { animationColorSpace: 'RGB', valueTemplate: undefined, fromValue: 'rgba(172,172,172,0)'}], position: 0, duration: 500 },
                { id: "eid21", tween: [ "color", "${_Text}", "color", 'rgba(255,255,255,1.00)', { animationColorSpace: 'RGB', valueTemplate: undefined, fromValue: 'rgba(134,134,134,1.00)'}], position: 500, duration: 62, easing: "easeInQuad" },
                { id: "eid19", tween: [ "style", "${_Text}", "left", '13px', { fromValue: '563px'}], position: 0, duration: 500, easing: "easeOutQuad" }            ]
        }
    }
}
};


Edge.registerCompositionDefn(compId, symbols, fonts, resources, opts);

/**
 * Adobe Edge DOM Ready Event Handler
 */
$(window).ready(function() {
     Edge.launchComposition(compId);
});
})(jQuery, AdobeEdge, "EDGE-1117821532");

/*
    CodeMirrorBase is a clientside only component that can not be used in a isomorphic environment.
    Within our Next project, you will be required to use the <CodeMirror/> component instead, which
    will lazily import the CodeMirrorBase via Next's `dynamic` component loading mechanisms.

    ! If you load this component in a Next page without using dynamic loading, you will get a SSR error !

    This file is providing the core functionality and logic of our CodeMirror instances.
 */

%raw
{|
import "codemirror/lib/codemirror.css";
import "../styles/cm.css";

if (typeof window !== "undefined" && typeof window.navigator !== "undefined") {
  require("codemirror/mode/javascript/javascript");
  require("../plugins/cm-reason-mode");
}
|};

/* The module for interacting with the imperative CodeMirror API */
module CM = {
  type t;

  let errorGutterId = "errors";

  module Options = {
    [@bs.deriving {abstract: light}]
    type t = {
      theme: string,
      [@bs.optional]
      gutters: array(string),
      mode: string,
      [@bs.optional]
      lineNumbers: bool,
      [@bs.optional]
      readOnly: bool,
      [@bs.optional]
      lineWrapping: bool,
    };
  };

  [@bs.module "codemirror"]
  external fromTextArea: (Dom.element, Options.t) => t = "fromTextArea";

  [@bs.send]
  external getScrollerElement: t => Dom.element = "getScrollerElement";

  [@bs.send]
  external getWrapperElement: t => Dom.element = "getWrapperElement";

  [@bs.send] external refresh: t => unit = "refresh";

  [@bs.send]
  external onChange:
    (t, [@bs.as "change"] _, [@bs.uncurry] (t => unit)) => unit =
    "on";
  [@bs.send] external toTextArea: t => unit = "toTextArea";

  [@bs.send] external setValue: (t, string) => unit = "setValue";

  [@bs.send] external getValue: t => string = "getValue";

  [@bs.send]
  external operation: (t, [@bs.uncurry] (unit => unit)) => unit = "operation";

  [@bs.send]
  external setGutterMarker: (t, int, string, Dom.element) => unit =
    "setGutterMarker";

  [@bs.send] external clearGutter: (t, string) => unit;

  type markPos = {
    line: int,
    ch: int,
  };

  module TextMarker = {
    type t;

    [@bs.send] external clear: t => unit = "clear";
  };

  module MarkTextOption = {
    type t;

    [@bs.obj] external make: (~className: string=?, unit) => t;
  };

  [@bs.send]
  external markText: (t, markPos, markPos, MarkTextOption.t) => TextMarker.t =
    "markText";
};

module DomUtil = {
  [@bs.val] [@bs.scope "document"]
  external createElement: string => Dom.element = "createElement";

  [@bs.send]
  external appendChild: (Dom.element, Dom.element) => unit = "appendChild";

  [@bs.set] [@bs.scope "style"]
  external setMinHeight: (Dom.element, string) => unit = "minHeight";

  [@bs.set] [@bs.scope "style"]
  external setDisplay: (Dom.element, string) => unit = "display";

  [@bs.set] [@bs.scope "style"]
  external setTop: (Dom.element, string) => unit = "top";

  [@bs.set] [@bs.scope "style"]
  external setLeft: (Dom.element, string) => unit = "left";

  [@bs.set] external setInnerHTML: (Dom.element, string) => unit = "innerHTML";

  [@bs.set] external setClassName: (Dom.element, string) => unit = "className";

  [@bs.set]
  external setOnMouseOver: (Dom.element, unit => unit) => unit = "onmouseover";

  [@bs.set]
  external setOnMouseLeave: (Dom.element, unit => unit) => unit =
    "onmouseleave";

  type clientRect = {
    x: int,
    y: int,
    width: int,
    height: int,
  };

  [@bs.send]
  external getBoundingClientRect: Dom.element => clientRect =
    "getBoundingClientRect";
};

type cmError = {
  row: int,
  column: int,
  endRow: int,
  endColumn: int,
  text: string,
};

module Props = {
  [@bs.deriving {abstract: light}]
  type t = {
    [@bs.optional]
    errors: array(cmError),
    [@bs.optional]
    minHeight: string, // minHeight of the scroller element
    [@bs.optional]
    className: string,
    [@bs.optional]
    style: ReactDOMRe.Style.t,
    value: string,
    [@bs.optional]
    onChange: (string) => unit,
    options: CM.Options.t,
  };
};

module ErrorMarker = {
  // Note: this is not a React component
  let make =
      (
        ~id,
        ~text: string,
        ~onMouseHover=?,
        ~onMouseLeave=?,
        ~wrapper: Dom.element,
        (),
      )
      : Dom.element => {
    open DomUtil;

    let msg = createElement("div");
    msg->setClassName("z-10 absolute px-2 rounded bg-fire-15");
    msg->setInnerHTML(text);
    msg->setDisplay("none");

    let showMsg = () => {
      msg->setDisplay("block");
    };

    let hideMsg = () => {
      msg->setDisplay("none");
    };

    let marker = createElement("div");
    marker->setClassName(
      "text-center text-fire font-bold bg-fire-15 rounded-full w-5 h-5 ml-2 hover:cursor-pointer",
    );
    marker->setInnerHTML("!");

    marker->setOnMouseOver(() => {
      let wrapperPos = wrapper->getBoundingClientRect;
      let pos = marker->getBoundingClientRect;
      Js.log(text);

      msg->setLeft(
        Belt.Int.toString(pos.x - wrapperPos.x + pos.width) ++ "px",
      );
      msg->setTop(Belt.Int.toString(pos.y - wrapperPos.y) ++ "px");

      Belt.Option.forEach(onMouseHover, cb => cb(id));
      showMsg();
    });

    marker->setOnMouseLeave(() => {
      Belt.Option.forEach(onMouseLeave, cb => cb(id));
      hideMsg();
    });

    marker;
  };
};

type state = {marked: array(CM.TextMarker.t)};

let clearMarks = (state: state): unit => {
  Belt.Array.forEach(state.marked, mark => {mark->CM.TextMarker.clear});
};

let updateErrors = (~state: state, ~cm: CM.t, errors) => {
  clearMarks(state);
  cm->CM.(clearGutter(errorGutterId));

  let wrapper = cm->CM.getWrapperElement;

  Belt.Array.forEachWithIndex(
    errors,
    (idx, e) => {
      open DomUtil;

      let onMouseHover = id => {
        Js.log2("onMouseHover: ", id);
      };
      let onMouseLeave = id => {
        Js.log2("onMouseLeave: ", id);
      };
      let marker =
        ErrorMarker.make(
          ~id=idx,
          ~onMouseHover,
          ~onMouseLeave,
          ~text=e.text,
          ~wrapper,
          (),
        );
      wrapper->appendChild(marker);

      // CodeMirrors line numbers are (strangely enough) zero based
      let row = e.row - 1;
      let endRow = e.endRow - 1;

      cm->CM.setGutterMarker(row, CM.errorGutterId, marker);

      let from = {CM.line: row, ch: e.column};
      let to_ = {CM.line: endRow, ch: e.endColumn};

      cm
      ->CM.markText(
          from,
          to_,
          CM.MarkTextOption.make(~className="bg-fire-15", ()),
        )
      ->Js.Array2.push(state.marked, _)
      ->ignore;
      ();
    },
  );
};

let default = (props: Props.t): React.element => {
  let inputElement = React.useRef(Js.Nullable.null);
  let cmRef: React.Ref.t(option(CM.t)) = React.useRef(None);
  let cmStateRef = React.useRef({marked: [||]});

  // Destruct all our props here
  let minHeight = props->Props.minHeight;
  let onChange = props->Props.onChange;
  let value = props->Props.value;
  let errors = Belt.Option.getWithDefault(props->Props.errors, [||]);
  let className = props->Props.className;
  let style = props->Props.style;
  let cmOptions = props->Props.options;

  React.useEffect0(() => {
    switch (inputElement->React.Ref.current->Js.Nullable.toOption) {
    | Some(input) =>
      let options =
        CM.Options.t(
          ~theme="material",
          ~gutters=[|CM.errorGutterId, "CodeMirror-linenumbers"|],
          ~mode=cmOptions->CM.Options.mode,
          ~lineWrapping=
            Belt.Option.getWithDefault(
              cmOptions->CM.Options.lineWrapping,
              false,
            ),
          ~readOnly=
            Belt.Option.getWithDefault(cmOptions->CM.Options.readOnly, false),
          ~lineNumbers=true,
          (),
        );
      let cm = CM.fromTextArea(input, options);
      /*Js.log2("cm", cm);*/

      Belt.Option.forEach(minHeight, minHeight =>
        cm->CM.getScrollerElement->DomUtil.setMinHeight(minHeight)
      );

      Belt.Option.forEach(onChange, onValueChange => {
        cm->CM.onChange(instance => {
          onValueChange(instance->CM.getValue);
        })
      });

      // For some reason, injecting value with the options doesn't work
      // so we need to set the initial value imperatively
      cm->CM.setValue(value);

      React.Ref.setCurrent(cmRef, Some(cm));

      let cleanup = () => {
        /*Js.log2("cleanup", options->CM.Options.mode);*/

        // This will destroy the CM instance
        cm->CM.toTextArea;
        React.Ref.setCurrent(cmRef, None);
      };

      Some(cleanup);
    | None => None
    }
  });

  /*
     Previously we did this in a useEffect([|value|) setup, but
     this issues for syncing up the current editor value state
     with the passed value prop.

     Example: Let's assume you press a format code button for a
     piece of code that formats to the same value as the previously
     passed value prop. Even though the source code looks different
     in the editor (as observed via getValue) it doesn't recognize
     that there is an actual change.

     By checking if the local state of the CM instance is different
     to the input value, we can sync up both states accordingly
   */
  switch (cmRef->React.Ref.current) {
  | Some(cm) =>
    if (CM.getValue(cm) === value) {
      ();
    } else {
      let state = cmStateRef->React.Ref.current;
      cm->CM.operation(() => {updateErrors(~state, ~cm, errors)});
      cm->CM.setValue(value);
    }
  | None => ()
  };

  React.useEffect1(
    () => {
      let state = cmStateRef->React.Ref.current;
      switch (cmRef->React.Ref.current) {
      | Some(cm) =>
        cm->CM.operation(() => {updateErrors(~state, ~cm, errors)})
      | None => ()
      };
      None;
    },
    [|errors|],
  );

  <div ?className ?style>
    <textarea className="hidden" ref={ReactDOMRe.Ref.domRef(inputElement)} />
  </div>;
};

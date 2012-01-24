<%doc>
  Generic page for a tool
</%doc>


<%inherit file="fluid.mako"/>

<%def name="css_slot()">
${h.stylesheet_link(request.static_url("pawsapp:static/css/jq/jquery-linedtextarea.css"), media="all")}
${h.stylesheet_link(request.static_url("pawsapp:static/css/jq/jquery.jqzoom.css"), media="all")}
${h.stylesheet_link(request.static_url("pawsapp:static/css/pygments-min.css"), media="all")}
</%def>

<%def name="js_slot()">
${h.javascript_link(request.route_url("routes.js"))}
${h.javascript_link(request.static_url("pawsapp:static/jq/bootstrap-tabs.js"))}
${h.javascript_link(request.static_url("pawsapp:static/jq/bootstrap-modal.js"))}
${h.javascript_link(request.static_url("pawsapp:static/jq/jquery-linedtextarea.js"))}
${h.javascript_link(request.static_url("pawsapp:static/jq/jquery.jqzoom-core.js"))}
<script type="text/javascript">
  operation = "${tool}";
</script>
</%def>


## LEFT COLUMN

<%def name="left_column()">

<div id="resizing_source" style="text-align:right">
  ${h.link_to(u"A+", id="aplus", class_="btn small")}
  ${h.link_to(u"A-", id="aminus", class_="btn small")}
</div>

<h5>Type or select source code from:</h5>

<dl>
  <dt>A set of classic examples:</dt>
  <dd>
    <a class="btn small" data-controls-modal="classic-examples-dialog" data-backdrop="static">BROWSE</a>
    <br/><br/>
  </dd>
  <dt>Or from your own test cases:</dt>
  <dd>
    <input type="submit" value="BROWSE" id="pseudobutton"/>
    <input type="file" id="inp" name="file" class="hide"/>
    <input type="text" id="pseudotextfile" readonly="readonly" class="span3"/>
  </dd>
</dl>

<p>${h.link_to(u"RUN", id="run-button", class_="btn small")}</p>
<p>${h.link_to(u"SAVE RESULT",  id="save-button",  class_="btn small disabled")}</p>
<p>${h.link_to(u"PRINT RESULT", id="print-button", class_="btn small disabled")}</p>

<div>
  % if advanced:
  ${h.link_to(u"Basic mode", url=request.route_url("tool_basic", tool=tool))}
  % else:
  ${h.link_to(u"Advanced mode", url=request.route_url("tool_advanced", tool=tool))}
  % endif
</div>

</%def>


## MAIN COLUMN

<%def name="main_column()">

<iframe id="iframetoprint" style="height: 0px; width: 0px; position: absolute; -moz-opacity: 0; opacity: 0"></iframe>

<h2 style="padding-bottom:1em">${descr}
% if advanced:
(advanced)
% endif
</h2>

<div id="op-tabs">

  ## Tab headers
  <ul class="tabs" data-tabs="tabs">
    <li class="active"><a href="#tabs-1" id="source_tab_link1">SOURCE</a></li>
    <li><a href="#result" id="result_tab_link">${tool.upper()}</a></li>
    <li><a href="#graph"  id="graph_tab_link">GRAPH</a></li>
  </ul>
  
  <div class="tab-content">

    ## Source code panel
    <div id="tabs-1" class="active tab-pane">
      <p>
	<label for="lang1">Language: </label>
	<input id="lang1" value="not yet detected." readonly="readonly"/>
      </p>
      <textarea id="sourcecode1" rows="27" onkeydown="handle_keydown(this, event)">Put your source code here.</textarea>
    </div>

    ## Result panel
    <div id="result" class="tab-pane">
      <div id="multiple-functions">
      </div>
      <div id="resultcode">
	Result of the transformation will be displayed here.
      </div>
    </div>

    ## Graph panel
    <div id="graph" class="tab-pane">
      <div id="dependence_graph">
	Please wait, it might take a long time.
      </div>
    </div>

  </div>
</div>
</%def>


## DIALOG BOXES

<%def name="dialogs()">

## Classic examples dialog

<div class="modal hide fade" id="classic-examples-dialog" style="display: none;">
  <div class="modal-header">
    <a class="close" href="#">×</a>
    <h3>Please select an example</h3>
  </div>
  <div class="modal-body">
    % for ex in examples:
    ${h.link_to(ex, id=ex, class_="btn small span8")}<br/>
    % endfor
  </div>
</div>

<div id="dialog-choose-function" title="Select function to transform.">
  <div class="choose-function" id="choose-function-buttons"></div>
</div>

</%def>

-- Minimal UI for the test faction. The base UI::update() expects a settings
-- panel that only real UIs create, so it is overridden with a no-op.
return {
	update = function() end,
}

from validation import vworkspace

with vworkspace() as w:
    w.props.memory_effects_only = False
    w.all_functions.validate_phases("print_code_proper_effects")


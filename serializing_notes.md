 note on serializing using a non json format. 


store floats in hex to avoid precision issues.
mark values that are not the default with a * so that when loading we can skip an entire type if it has not changed 

have a version number, 


format {
    version: 15;

    example_field  = defaultvalue; @v1 
    versiontwofield = defaultvalue; @v2 
    versiontwoalso = defaultvalue; @v2 
    new_value_n_three = defaultvalue; @v3
    deprecated_value = defaultvalue; @v3-4
    another_new_value_n_three = defaultvalue; @v3
    thing_we_needed_but_not_really = default; @v4
}


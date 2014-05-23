i3_project(PulseCore
  PYTHON_DIR python
  PYTHON_DEST icecube/PulseCore  
  DOCS_DIR resources/docs
  )

i3_add_library(PulseCore
  private/PulseCore/*.cxx 
  public/PulseCore/*.h
  USE_TOOLS boost python

  USE_PROJECTS icetray dataclasses

  )


namespace eval tut {

sta::define_cmd_args "print" {}
proc init {} {
  print
}

sta::define_cmd_args "printCells" {}
proc init {} {
  printCells
}

sta::define_cmd_args "printNets" {}
proc init {} {
  printNets
}

sta::define_cmd_args "printPins" {}
proc init {} {
  printPins
}

sta::define_cmd_args "printHPWLs" {}
proc init {} {
  printHPWLs
}

# tut namespace end
}

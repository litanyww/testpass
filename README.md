# Testpass

This is a tool to facilitate manual testing.

It is intended to be a very light-weight, command-line based solution.  It is
expected to be broadly cross platform, though at present work is needed to
support systems which do not support `fork()`/`exec()`.  It is driven by discrete
test steps, each of which may have dependencies on other test steps.

I'm hoping the tool is useful in its own right, but this project is very new,
and I'm making it available now in order to gather feedback.  Feedback is
welcomed, both positive and negative.

## Test Steps

Most manual test case management systems encourage breaking test cases up into
individual test steps.  This project takes this to an extreme and discards the
concept of test case entirely.

Test steps are defined in individual files, one test step per file.  The test
step definition file will declare the following information:


| field        | description |
| ------------ | -------------- |
| short        | A short name for the test step; recommended to match the filename |
| requirements | comma separated list of attributes indicating the system state required for this test step to be run |
| changes      | comma separated list of attributes indicating how the system state will be changed when this test step is run |
| cost         | An integer value which indicates the relative time or cost taken to perform this test step. |
| required     | Whether this is a required test step, or will only be included if this step is needed by a required test step *(default: yes)* |
| description  | Instructions on how to perform the test step |
| script       | shell script which can optionally be run to assist with, or replace the manual test step. |

The required status of a step may be overridden by specifying the directories
containing required test steps on the command line.  All the test steps within
these directories become required; otherwise required test steps from other
directories will no longer be treated as required test steps.  In this way, it
is sensible to distribute your test cases among a hierarchy of directories,
making it easy to tailor your test plan based on broad areas of interest.

## Attributes

Each test step may declare a dependency on a given system state defined by a
series of 'attributes'.  Each step may also modify the system state by setting
or unsetting these attributes.

In its simplest form, an attribute may be simply be any word.  An attribute may
be declared as a dependency by adding it to a comma separated list for the
'dependency' field.  To forbid that step from running while that attribute is
present in the current state, preceed it with a '!'.

For example:

```
dependencies: installed, !active
```

A test step with such a declared dependency would require that the system have
the 'installed' attribute set, and must not have the 'active' state set.

A test step may add a specific attribute to the current state by adding it to
the comma-separated list of attributes on its 'changes' field.  To remove a
given attribute from the system state, preceed it with a '!'

For example:

```
changes: installed, !clean
```

This would add the 'installed' attribute to the system state, and remove
`clean` if it was present.

An attribute may alse be of the form `key=value`.  Each test step may declare
dependencies on such a compound attribute, and may change the state exactly as
for simple attributes, but the system state will only allow one compound
attribute for each key to be in the system state at any one time.

It therefore makes no sense for a test step to declare multiple compound
elements with the same key in its 'changes' field, however it is perfectly
sensible to include multiple compound attributes in its dependencies.  Note
that this may cause the step to be executed multiple times, which in most cases
is exactly what you'll want.

## Advantages

 - Test steps are simple text files, and can be managed using VCS tools -
   potentially alongside your source.
 - Test steps can be partially or fully automated.
 - Test dependencies need only be defined once, and will automatically be
   shared by any test step which requires it.
 - Test plans are constructed with a test step order which minimises the setup
   costs for each step.  Test steps which share similar prerequisites will be
   grouped together.
 - Test steps are easily edited.
 - It is easy to add a new variation of a test step, which previously might
   have been handled by cloning an existing test case, or laboriously inserting
   the variation and its prerequisite steps into an existing test case.
 - When executing a test plan, a simple text based log file is generated.
 - A test pass run can be interrupted and restarted at any time.
 - A restarted test pass will not re-run test cases present in the log file.
   Edit the log file to remove test cases which need to be re-run.
 - The system state is included in the log file, but can be overridden when
   restarting a test pass.

## Example test case

```
short: testDarkConfigInLight
dependencies: darkConfig=one, darkConfig=two, light
changes: tested
cost: 1
required: yes
description::
This is a description of how to perform the test step
.
script::
echo "this is a script"
.
```

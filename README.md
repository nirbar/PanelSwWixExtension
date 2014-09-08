# README #

PanelSwWixExtension is a [WiX](http://wixtoolset.org/) extension that contains various addition to the build-in WiX toolset:

WiX Elements:
* *ReadIniValues*: Reads .INI file values.
* *CustomUninstallKey*: Overwrite registry values in the product's [Uninstall](http://msdn.microsoft.com/en-us/library/aa372105%28v=vs.85%29.aspx) registry key

Custom Actions:
* *TerminateSuccessfully_Immediate*: Terminates the installation with a successful indication. Executed in-script.
* *TerminateSuccessfully_Deferred*: Terminates the installation with a successful indication. Deferred execution.

To build the extension download the code, open the solution PanelSwWixExtension.sln and build it in Visual Studio.

Unit-test WiX are available in the solution folder 'UnitTests'. 

PanelSwWixExtension is the most comprehensive open source [WiX](http://wixtoolset.org/) extension. 
It features dozens of actions from querying XML, JSON and text files to installing [ServiceBase](https://docs.microsoft.com/en-us/dotnet/api/system.serviceprocess.servicebase)-based and [TopShelf](http://topshelf-project.com/) services.

The commercial extension- [JetWixExtension](https://github.com/nirbar/JetBA-Showcase)- complements PanelSwWixExtension with the following features:
- JetBA: The most comprehensive, fully customizable, extensible, WPF-based BootstrapperApplication.
- JetBA++: The only native fully customizable, extensible, Qt-based BootstrapperApplication.
- Preprocessor extension:
  - Harvest directly from WiX source code by executing Heat commands. The command is passes to heat.exe with the following changes:
    - Added support to use DuplicateFile where possible:
      - -cp NameSizeHash: Files with the same name, size, and MD5 hash are duplicated
      - -cp NameVersion: File with the same name and version are duplicated. Files without a version are handled on NameSizeHash policy
    - The "-out" parameter can be omitted
    - "SourceDir" is replaced with preprocessed folder path in File/@Source and Payload/@SourceFile attributes
    ~~~~~~~
    <?pragma heat.dir "$(sys.SOURCEFILEDIR)..\bin\Release" -cg BIN -dr INSTALLFOLDER -ag?>
    ~~~~~~~
  - Collection variables
    ~~~~~~~
    <?pragma tuple.NIR a; b; c?>
    <?pragma tuple.BAR x; y; z?>
    <?foreach i in $(tuple_range.BAR())?>
    <Component Feature="ProductFeature" Directory="INSTALLFOLDER">
      <File Source="$(sys.SOURCEFILEPATH)" Name="Product.$(tuple.NIR($(var.i))).$(tuple.BAR($(var.i))).wxs"/>
    </Component>
    <?endforeach?>
    <?pragma endtuple.BAR?>
    <?pragma endtuple.NIR?>
    ~~~~~~~
  - Generate random Id. 
    Useful when deploying files with same name to different target folders:
    ~~~~~~~
  	<ComponentGroup Id="random">
  		<Component Directory="Product.Dir">
  		<File Source="$(sys.SOURCEFILEPATH)" Id="$(jet.RandomId())"/>
  		</Component>
  		<Component Directory="INSTALL_FOLDER">
  		<File Source="$(sys.SOURCEFILEPATH)" Id="$(jet.RandomId())"/>
  		</Component>
  	</ComponentGroup>
    ~~~~~~~
  - Get a running index. 
    On each call get the next index, starting at 1.
    ~~~~~~~
  	<ComponentGroup Id="random">
  		<Component Directory="Product.Dir">
  		<File Source="$(sys.SOURCEFILEPATH)" />
            <util:XmlFile ... Sequence="$(jet.Index())"/>
            <util:XmlFile ... Sequence="$(jet.Index())"/>
  		</Component>
  	</ComponentGroup>
    ~~~~~~~
  - Generate random or pseudo-random Guid. 
    ~~~~~~~
  	<ComponentGroup Id="random">
      <?foreach account in localService;localSystem;networkService;applicationPoolIdentity?>
      <Component Id="webapp.$(var.account)" Guid="$(jet.Guid($(var.account)))" Transitive="yes">
        <Condition><![CDATA[IIS_ACCOUNT ~= "$(var.account)"]]></Condition>
        <CreateFolder />
        <iis:WebAppPool Id="$(var.account)" Name="WebUI" Identity="$(var.account)" ManagedRuntimeVersion="v4.0" ManagedPipelineMode="Integrated" />
        <iis:WebVirtualDir Id="$(var.account)" Directory="INSTALLFOLDER" Alias="WebUI" WebSite="WebUI">
          <iis:WebApplication Id="$(var.account)" Name="WebUI" WebAppPool="$(var.account)" />
        </iis:WebVirtualDir>
      </Component>
      <?endforeach?>
  	</ComponentGroup>
    ~~~~~~~
- Detect bundles and get their versions in a variable:
    ~~~~~~~
    <jet:BundleSearch UpgradeCode="{1D1DB5E6-E0D8-3103-8570-369A82A9BF33}" VersionVariable="DetectedVC2013x86Version" NamePattern="\bx86\b"/>
    ~~~~~~~
- RebootBoundary attrbiute on bundle packages: force reboot after the package if any preceding package required reboot
Contact the owner to obtain a license for [JetWixExtension](https://github.com/nirbar/JetBA-Showcase)

## PanelSwWixExtension WiX Elements

- Immediate Actions:
  - *PathSearch*: Search for a file on PATH environment variable folder list.
  - *VersionCompare*: Compare two versions, set result to property as -1, 0, or 1
  - *ForceVersion*: Force a specified version for a file. Version overwrite is done on runtime in the MSI database.
  - *AccountSidSearch*: Lookup an account's SID by account name.
  - *JsonJpathSearch*: Read JSON values
  - *DiskSpace*: Calculate available disk space for a target directory
  - *CertificateHashSearch*: Find a certificate has in local system MY store.
  - *Evaluate*: Evaluate mathematical expressions and store result in property.
  - *SetPropertyFromPipe*: Allows setting property from a pipe.
  - *SqlSearch*: Execute a SQL query and place result in a property.
  - *ReadIniValues*: Reads .INI file values.
  - *XmlSearch*: Read XML values.
  - *RegularExpression*: Execute a Regular Expression to replace or find matches in property values and in file contents.
  - *MsiSqlQuery*: Execute a MSI-SQL query on the MSI database.
  - *CreateSelfSignCertificate*: Create a self-sign certificate that can then be installed by WixIisExtension.
- Deferred Actions:
  - *XslTransform*: Apply a XSL transform on an installed XML file
  - *WebsiteConfig*: currently, can only stop a website
  - *JsonJPath*: Set values in JSON-formatted file.
  - *Dism*: Enable Windows Feature using DISM API. Features will be installed when the parent component is being installed or repaired.
  - *ZipFile*: Creates a ZIP archive from selected files.
  - *Unzip*: Extract a ZIP archive to selected folder.
  - *CustomUninstallKey*: Overwrite registry values in the product's [Uninstall](http://msdn.microsoft.com/en-us/library/aa372105%28v=vs.85%29.aspx) registry key
  - *RemoveRegistryValue*: Removes registry values. Complements the standard [RemoveRegistryValue](http://wixtoolset.org/documentation/manual/v3/xsd/wix/removeregistryvalue.html) WiX element that will only remove registry values during installation.
  - *ExecOn*: Execute a custom command on component action.
  - *TaskScheduler*: Add a task to Windows Task Scheduler. Task definition XML should be in inner text.
  - *DeletePath*: Delete folder or file specified by a path.
  - *FileRegex*: Execute a Regular Expression to perform find & replace operations within files.
  - *ShellExecute*: Call ShellExecuteEx with parameters.
  - *Telemetry*: Send telemetry data to a given URL.
  - *InstallUtil*: Install a .NET assembly service.
  - *TopShelf*: Install a [TopShelf](http://topshelf-project.com/) based service.
  - *BackupAndRestore*: Backup a file before install or upgarde and restore it after.
  - *SqlScript*: Execute SQL scripts, optionally with text replacements.

## Custom Actions

- *TerminateSuccessfully_Immediate*: Terminates the installation with a successful indication. Executed in-script.
- *TerminateSuccessfully_Deferred*: Terminates the installation with a successful indication. Deferred execution.
- *SplitPath*: Split a full path supplied in 'FULL_PATH_TO_SPLIT' property and places the parts in properties SPLIT_DRIVE, SPLIT_FOLDER, SPLIT_FILE_NAME, SPLIT_FILE_EXTENSION.
- *PathExists*: Tests whether the path supplied in 'FULL_PATH_TO_TEST' property exists. Sets 'PATH_EXISTS' property to 1 if it exists or clears it if it doesn't.
- *SplitString*: Splits a string. Property name specified in 'PROPERTY_TO_SPLIT' and split token specified in 'STRING_SPLIT_TOKEN'. Results are stored in properties following the name supplied in 'PROPERTY_TO_SPLIT'.
  For example, say PROPERTY_TO_SPLIT="MY_PROP", MY_PROP="1,2,3" and STRING_SPLIT_TOKEN=",". Scheduling SplitString custom action will yield properties MY_PROP_0="1", MY_PROP_1="2" and MY_PROP_2="3".
- *TrimString*: Trim whitespace characters in property with name specified in 'PROPERTY_TO_TRIM'.

## Error codes

PanelSwWixExtension uses error codes in Error table:

- 27000: TopShelf error template for prompting user on errors.
- 27001: ExecOn error template for prompting user on errors.
- 27002: ServiceConfig error template for prompting user on errors.
- 27003: Dism error template for prompting user on failures to add a Windows feature package.
- 27004: Dism error template for prompting user on failures to enable a Windows feature.
- 27005: SqlScript error template for prompting user on errors.
- 27006: ExecOn error template for prompting user on console output parsing.
- 27007: WebsiteConfig error template for prompting user on errors.
- 27008: XslTransform error template for prompting user on errors.

## Properties

The following properties hold localized built-in account names. To use them, add a [PropertyRef](http://wixtoolset.org/documentation/manual/v3/xsd/wix/propertyref.html) element.

- DOMAIN_ADMINISTRATORS
- DOMAIN_GUESTS
- DOMAIN_USERS
- ENTERPRISE_DOMAIN_CONTROLLERS
- DOMAIN_DOMAIN_CONTROLLERS
- DOMAIN_COMPUTERS
- BUILTIN_ADMINISTRATORS
- BUILTIN_GUESTS
- BUILTIN_USERS
- LOCAL_ADMIN
- LOCAL_GUEST
- ACCOUNT_OPERATORS
- BACKUP_OPERATORS
- PRINTER_OPERATORS
- SERVER_OPERATORS
- AUTHENTICATED_USERS
- PERSONAL_SELF
- CREATOR_OWNER
- CREATOR_GROUP
- LOCAL_SYSTEM
- POWER_USERS
- EVERYONE
- REPLICATOR
- INTERACTIVE
- NETWORK
- SERVICE
- RESTRICTED_CODE
- WRITE_RESTRICTED_CODE
- ANONYMOUS
- SCHEMA_ADMINISTRATORS
- CERT_SERV_ADMINISTRATORS
- RAS_SERVERS
- ENTERPRISE_ADMINS
- GROUP_POLICY_ADMINS
- ALIAS_PREW2KCOMPACC
- LOCAL_SERVICE
- NETWORK_SERVICE
- REMOTE_DESKTOP
- NETWORK_CONFIGURATION_OPS
- PERFMON_USERS
- PERFLOG_USERS
- IIS_USERS
- CRYPTO_OPERATORS
- OWNER_RIGHTS
- EVENT_LOG_READERS
- ENTERPRISE_RO_DCs
- CERTSVC_DCOM_ACCESS

## Building

PanelSwWixExtension require the following prerequisites to build:
- CMake: CMake path can be specified in 'CMakeDir' property in 'TidyBuild.custom.props'
- ADK installed. Specifically, Dism API should be installed. Set DismApi folder path in 'DismApiDir' property in 'TidyBuild.custom.props'
- Update git submodules

To build the extension download the code, open the solution PanelSwWixExtension.sln in Visual Studio 2017 and build it.

Unit-test WiX are available in the solution folder 'UnitTests'.
After building a unit test project, you'll need to shutdown Visual Studio before you can build PanelSwWixExtension project again. 
This is due to the unfortunate habit of Visual Studio to hold the extension file in use.
You may find it convenient to build unit test projects from a command prompt to workaround this limitation
~~~~~~~~~~~~
MSBuild UnitTests\XmlSearchUT\XmlSearchUT.wixproj /p:Configuration=Release /p:Platform=x86 /t:Rebuild "/p:SolutionDir=%CD%\\"
~~~~~~~~~~~~
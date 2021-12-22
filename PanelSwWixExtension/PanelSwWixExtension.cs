namespace PanelSw.Wix.Extensions
{
    using System;
    using System.Reflection;

    using Microsoft.Tools.WindowsInstallerXml;

    /// <summary>
    /// A wix extension for PanelSwWixExtension custom action library.
    /// </summary>
    public sealed class PanelSwWixExtension : WixExtension
    {
        private Library library;
        private PanelSwWixCompiler compilerExtension;
        private PanelSwWixBinder binder_;
        private PanelSwWixUnbinder unbinder_;
        private TableDefinitionCollection tableDefinitions;

        public override BinderExtension BinderExtension => binder_ ?? (binder_ = new PanelSwWixBinder());
        public override UnbinderExtension UnbinderExtension => unbinder_ ?? (unbinder_ = new PanelSwWixUnbinder());

        /// <summary>
        /// Gets the optional compiler extension.
        /// </summary>
        /// <value>The optional compiler extension.</value>
        public override CompilerExtension CompilerExtension
        {
            get
            {
                if (null == this.compilerExtension)
                {
                    this.compilerExtension = new PanelSwWixCompiler();
                }

                return this.compilerExtension;
            }
        }

        /// <summary>
        /// Gets the optional table definitions for this extension.
        /// </summary>
        /// <value>The optional table definitions for this extension.</value>
        public override TableDefinitionCollection TableDefinitions
        {
            get
            {
                if (null == this.tableDefinitions)
                {
                    this.tableDefinitions = LoadTableDefinitionHelper(Assembly.GetExecutingAssembly(), "PanelSw.Wix.Extensions.Data.tables.xml");
                }

                return this.tableDefinitions;
            }
        }

        /// <summary>
        /// Gets the library associated with this extension.
        /// </summary>
        /// <param name="tableDefinitions">The table definitions to use while loading the library.</param>
        /// <returns>The library for this extension.</returns>
        public override Library GetLibrary(TableDefinitionCollection tableDefinitions)
        {
            if (null == this.library)
            {
                this.library = LoadLibraryHelper(Assembly.GetExecutingAssembly(), "PanelSw.Wix.Extensions.Data.PanelSwWixExtension.wixlib", tableDefinitions);
            }

            return this.library;
        }

        /// <summary>
        /// Gets the default culture.
        /// </summary>
        /// <value>The default culture.</value>
        public override string DefaultCulture
        {
            get { return "en-us"; }
        }
    }
}

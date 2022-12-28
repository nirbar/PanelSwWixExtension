using System;
using System.Collections.Generic;

namespace PanelSw.Wix.Extensions
{
    /// <summary>
    /// A wix extension for PanelSwWixExtension custom action library.
    /// </summary>
    public sealed class PanelSwWixExtension : WixToolset.Extensibility.BaseExtensionFactory
    {
        protected override IReadOnlyCollection<Type> ExtensionTypes => new Type[]
        {
            typeof(PanelSwWiBackendBinder),
            typeof(PanelSwWixCompiler),
            typeof(PanelSwWixExtData),
        };
    }
}

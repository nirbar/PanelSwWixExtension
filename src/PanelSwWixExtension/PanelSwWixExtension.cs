using System;
using System.Collections.Generic;
using WixToolset.Extensibility;

namespace PanelSw.Wix.Extensions
{
    public sealed class PanelSwWixExtension : BaseExtensionFactory
    {
        private PanelSwWixPreprocessor _preprocessor = null;
        public PanelSwWixExtension()
        {
#if DEBUG
            System.Diagnostics.Debugger.Launch();
#endif
        }

        protected override IReadOnlyCollection<Type> ExtensionTypes => new Type[]
        {
            typeof(PanelSwWixPreprocessor),
            typeof(PanelSwWiBackendBinder),
            typeof(PanelSwWixCompiler),
            typeof(PanelSwWixExtData),
            typeof(PanelSwBurnBackendBinder),
            typeof(PanelSwOptimizer),
#if EnableZipContainer
            typeof(PanelSwBurnContainer),
#endif
        };

        public static string MY_EXTENSION_ID = "PanelSwBackendExtension";

        public override bool TryCreateExtension(Type extensionType, out object extension)
        {
            if ((extensionType == typeof(IExtensionData)) || (extensionType == typeof(PanelSwWixExtData)))
            {
                extension = new PanelSwWixExtData(_preprocessor);
                return true;
            }

            bool res = base.TryCreateExtension(extensionType, out extension);
            if (res && extension is PanelSwWixPreprocessor)
            {
                _preprocessor = extension as PanelSwWixPreprocessor;
            }

            return res;
        }
    }
}

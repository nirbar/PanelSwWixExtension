using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using WixToolset.Harvesters.Data;
using WixToolset.Harvesters.Extensibility;
using WixToolset.Harvesters.Serialize;

namespace PanelSw.Wix.HarvestExtension
{
    internal class PanelSwWixHarvestMutator : BaseMutatorExtension
    {
        public override int Sequence => 0;

        public string ExcludePatterns { get; set; }
        public string IncludePatterns { get; set; }
        public string PayloadRootFolder { get; set; }

        private List<File> _files = new List<File>();
        private List<Payload> _payloads = new List<Payload>();
        private List<ComponentRef> _componentrefs = new List<ComponentRef>();

        public override void Mutate(WixToolset.Harvesters.Serialize.Wix wix)
        {
            _files.Clear();
            _payloads.Clear();
            _componentrefs.Clear();

            Index(wix);

            MutatePayloads();
            MutateFiles();
        }

        private void MutateFiles()
        {
            foreach (File file in _files)
            {
                string fileName = System.IO.Path.GetFileName(file.Source);
                if ((string.IsNullOrEmpty(IncludePatterns) || IsFilenameMatch(fileName, IncludePatterns))
                    && (string.IsNullOrEmpty(ExcludePatterns) || !IsFilenameMatch(fileName, ExcludePatterns)))
                {
                    continue;
                }

                if (file.ParentElement is IParentElement parent)
                {
                    parent.RemoveChild(file);
                    if (parent is Component component)
                    {
                        RemoveComponent(component);
                    }
                }
            }
        }

        private void RemoveComponent(Component component)
        {
            if (component.ParentElement is IParentElement parent)
            {
                parent.RemoveChild(component);
            }
            ComponentRef componentRef = _componentrefs.FirstOrDefault(c => c.Id.Equals(component.Id));
            if (componentRef?.ParentElement is IParentElement parent1)
            {
                parent1.RemoveChild(componentRef);
            }
        }

        private void MutatePayloads()
        {
            foreach (Payload payload in _payloads)
            {
                if (!string.IsNullOrEmpty(PayloadRootFolder))
                {
                    if (string.IsNullOrEmpty(payload.Name))
                    {
                        payload.Name = System.IO.Path.GetFileName(payload.SourceFile);
                    }
                    payload.Name = System.IO.Path.Combine(PayloadRootFolder, payload.Name);
                }

                string fileName = System.IO.Path.GetFileName(payload.SourceFile);
                if ((string.IsNullOrEmpty(IncludePatterns) || IsFilenameMatch(fileName, IncludePatterns))
                    && (string.IsNullOrEmpty(ExcludePatterns) || !IsFilenameMatch(fileName, ExcludePatterns)))
                {
                    continue;
                }

                if (payload.ParentElement is IParentElement parent)
                {
                    parent.RemoveChild(payload);
                }
            }
        }

        private void Index(ISchemaElement element)
        {
            if (element is File file)
            {
                _files.Add(file);
            }
            else if (element is ComponentRef cref)
            {
                _componentrefs.Add(cref);
            }
            else if (element is Payload payload)
            {
                _payloads.Add(payload);
            }
            if (element is IParentElement parent)
            {
                foreach (ISchemaElement child in parent.Children)
                {
                    Index(child);
                }
            }
        }

        private enum PatternSpec
        {
            // The pszSpec parameter points to a single file name pattern to be matched.
            PMSF_NORMAL = 0,

            // The pszSpec parameter points to a semicolon-delimited list of file name patterns to be matched.
            PMSF_MULTIPLE = 1,

            // If PMSF_NORMAL is used, don't ignore leading spaces in the string pointed to by pszSpec.
            // If PMSF_MULTIPLE is used, don't ignore leading spaces in each file type contained in the string pointed to by pszSpec.
            // This flag can be combined with PMSF_NORMAL and PMSF_MULTIPLE
            PMSF_DONT_STRIP_SPACES = 0x00010000,
        }

        [DllImport("Shlwapi.dll", CharSet = CharSet.Unicode)]
        private static extern int PathMatchSpecExW([MarshalAs(UnmanagedType.LPWStr)] string szFile, [MarshalAs(UnmanagedType.LPWStr)] string szSpec, [MarshalAs(UnmanagedType.U4)] PatternSpec dwFlags);

        private bool IsFilenameMatch(string fileName, string pattern)
        {
            int res = PathMatchSpecExW(fileName, pattern, PatternSpec.PMSF_MULTIPLE);
            return (res == 0);
        }
    }

    internal class PanelSwWixHarvestExtension : BaseHeatExtension
    {
        public override HeatCommandLineOption[] CommandLineTypes =>
            new HeatCommandLineOption[]
            {
                new HeatCommandLineOption("exc", "Semicolon seperated list of filename to exclude. Wildcards are accepted"),
                new HeatCommandLineOption("inc", "Semicolon seperated list of filename to include. Wildcards are accepted. If specified, any file not matching the pattern will be excluded"),
                new HeatCommandLineOption("prd", "Payload Root Folder. A prefix folder to add to all payloads"),
            };

        public override void ParseOptions(string type, string[] args)
        {
            if (!type.Equals("dir", StringComparison.InvariantCultureIgnoreCase))
            {
                return;
            }

            PanelSwWixHarvestMutator mutator = new PanelSwWixHarvestMutator();
            for (int i = 0; i < args.Length - 1; ++i)
            {
                string arg = args[i];
                if (string.IsNullOrEmpty(arg) || (!arg.StartsWith("-") && !arg.StartsWith("/")))
                {
                    continue;
                }

                string patt = args[i + 1];
                if (string.IsNullOrEmpty(patt))
                {
                    continue;
                }

                arg = arg.Substring(1);
                switch (arg.ToLower())
                {
                    case "exc":
                        mutator.ExcludePatterns = patt;
                        break;
                    case "inc":
                        mutator.IncludePatterns = patt;
                        break;
                    case "prd":
                        mutator.PayloadRootFolder = patt;
                        break;
                }
            }

            if (!string.IsNullOrEmpty(mutator.ExcludePatterns) || !string.IsNullOrEmpty(mutator.IncludePatterns) || !string.IsNullOrEmpty(mutator.PayloadRootFolder))
            {
                Core.Mutator.AddExtension(mutator);
            }
        }
    }
}

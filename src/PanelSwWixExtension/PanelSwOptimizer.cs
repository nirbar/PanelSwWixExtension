using Microsoft.Extensions.FileSystemGlobbing;
using Microsoft.Extensions.FileSystemGlobbing.Abstractions;
using PanelSw.Wix.Extensions.Symbols;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Xml.Linq;
using WixToolset.Data;
using WixToolset.Data.Symbols;
using WixToolset.Extensibility;
using WixToolset.Extensibility.Data;
using WixToolset.Extensibility.Services;

namespace PanelSw.Wix.Extensions
{
    public class PanelSwOptimizer : BaseOptimizerExtension
    {
        private IParseHelper _parseHelper;
        private IMessaging _messaging;
        private IOptimizeContext _context;

        public override void PreOptimize(IOptimizeContext context)
        {
            _context = context;
            _parseHelper = _context.ServiceProvider.GetService<IParseHelper>();
            _messaging = _context.ServiceProvider.GetService<IMessaging>();
            base.PreOptimize(context);

            ResolveFileGlob();
        }

        private void ResolveFileGlob()
        {
            Regex bindPathRx = new Regex(@"!\(bindpath\.(?<name>[\w_]+)\).*", RegexOptions.Compiled);
            foreach (Intermediate intermediate in _context.Intermediates)
            {
                foreach (IntermediateSection section in intermediate.Sections)
                {
                    List<PSW_FileGlob> fileGlobs = new List<PSW_FileGlob>();
                    List<PSW_FileGlobPattern> fileGlobPatterns = new List<PSW_FileGlobPattern>();

                    foreach (IntermediateSymbol symbol in section.Symbols)
                    {
                        if (symbol is PSW_FileGlob glb)
                        {
                            fileGlobs.Add(glb);
                        }
                        if (symbol is PSW_FileGlobPattern patt)
                        {
                            fileGlobPatterns.Add(patt);
                        }
                    }

                    // Collect files
                    foreach (PSW_FileGlob glb in fileGlobs)
                    {
                        bool isBundle = !string.IsNullOrEmpty(glb.PayloadGroup_);
                        Matcher matcher = new Matcher();
                        IEnumerable<PSW_FileGlobPattern> patterns = fileGlobPatterns.Where(p => p.FileGlob_.Equals(glb.Id.Id));
                        matcher.AddIncludePatterns(patterns.Where(p => !string.IsNullOrEmpty(p.Include)).Select(p => p.Include));
                        matcher.AddExcludePatterns(patterns.Where(p => !string.IsNullOrEmpty(p.Exclude)).Select(p => p.Exclude));

                        List<string> baseFolders = new List<string>();
                        Match rxMatch = bindPathRx.Match(glb.SourceDir);
                        if (rxMatch.Success)
                        {
                            string bindName = rxMatch.Groups["name"].Value;
                            baseFolders = new List<string>(_context.BindPaths?.Where(b => bindName.Equals(b.Name))?.Select(b => b.Path));
                        }
                        else
                        {
                            baseFolders.Add(glb.SourceDir);
                        }

                        if ((baseFolders == null) || (baseFolders.Count == 0) || !baseFolders.Any(d => Directory.Exists(d)))
                        {
                            _messaging.Write(ErrorMessages.ExpectedDirectory(glb.SourceDir));
                            continue;
                        }

                        Dictionary<string, string> sectionCachedInlinedDirectoryIds = new Dictionary<string, string>();
                        foreach (string folder in baseFolders)
                        {
                            if (!Directory.Exists(folder))
                            {
                                continue;
                            }

                            PatternMatchingResult patternMatching = matcher.Execute(new DirectoryInfoWrapper(new DirectoryInfo(folder)));
                            foreach (FilePatternMatch filePattern in patternMatching.Files)
                            {
                                string fullPath = Path.Combine(folder, filePattern.Path);
                                fullPath = Path.GetFullPath(fullPath);

                                string recursiveDir = Path.GetDirectoryName(filePattern.Path);

                                if (!string.IsNullOrEmpty(glb.PayloadGroup_))
                                {
                                    Identifier id = _parseHelper.CreateIdentifier("glb", glb.PayloadGroup_, recursiveDir, Path.GetFileName(fullPath));
                                    string fileName = Path.Combine(recursiveDir, Path.GetFileName(fullPath));

                                    section.AddSymbol(new WixBundlePayloadSymbol(glb.SourceLineNumbers, id) { SourceFile = new IntermediateFieldPathValue() { Path = fullPath }, Name = fileName });
                                    section.AddSymbol(new WixGroupSymbol(glb.SourceLineNumbers, id) { ChildId = id.Id, ChildType = ComplexReferenceChildType.Payload, ParentId = glb.PayloadGroup_, ParentType = ComplexReferenceParentType.PayloadGroup });
                                }
                                else
                                {
                                    string directoryId = glb.Directory_;
                                    if (!string.IsNullOrEmpty(recursiveDir))
                                    {
                                        directoryId = _parseHelper.CreateDirectoryReferenceFromInlineSyntax(section, glb.SourceLineNumbers, null, directoryId, recursiveDir, sectionCachedInlinedDirectoryIds);
                                    }
                                    Identifier id = _parseHelper.CreateIdentifier("glb", directoryId, Path.GetFileName(fullPath));

                                    section.AddSymbol(new ComponentSymbol(glb.SourceLineNumbers, id)
                                    {
                                        ComponentId = "*",
                                        DirectoryRef = directoryId,
                                        KeyPath = id.Id,
                                        KeyPathType = ComponentKeyPathType.File,
                                        Location = ComponentLocation.LocalOnly,
                                        Win64 = _context.Platform == Platform.ARM64 || _context.Platform == Platform.X64,
                                    });

                                    section.AddSymbol(new FileSymbol(glb.SourceLineNumbers, id)
                                    {
                                        Source = new IntermediateFieldPathValue() { Path = fullPath },
                                        Name = Path.GetFileName(fullPath),
                                        ComponentRef = id.Id,
                                        DirectoryRef = directoryId,
                                        Attributes = FileSymbolAttributes.None | FileSymbolAttributes.Vital,
                                    });
                                    if (!string.IsNullOrEmpty(glb.ComponentGroup_))
                                    {
                                        _parseHelper.CreateComplexReference(section, glb.SourceLineNumbers, ComplexReferenceParentType.ComponentGroup, glb.ComponentGroup_, null, ComplexReferenceChildType.Component, id.Id, false);
                                    }
                                    if (!string.IsNullOrEmpty(glb.Feature_))
                                    {
                                        _parseHelper.CreateComplexReference(section, glb.SourceLineNumbers, ComplexReferenceParentType.Feature, glb.Feature_, null, ComplexReferenceChildType.Component, id.Id, true);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

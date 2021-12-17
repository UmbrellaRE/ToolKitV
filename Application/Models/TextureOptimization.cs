using CodeWalker.GameFiles;
using CodeWalker.Utils;
using System;
using System.Diagnostics;
using System.IO;

namespace ToolkitV.Models
{
    public partial class TextureOptimization
    {
        public struct StatsData
        {
            public int filesCount = 0;
            public int oversizedCount = 0;
            public float virtualSize = 0;
            public float physicalSize = 0;
        }

        public struct ResultsData
        {
            public float filesSize = 0;
            public int filesOptimized = 0;
            public float optimizedSize = 0;
            public float optimizedProcent = 0;
        }

        private static Texture OptimizeTexture(Texture texture, bool formatOptimization, bool downsize)
        {
            string savePath = "temp.dds";
            byte[] dds;

            texture.Levels = Convert.ToByte(Math.Log(Math.Min(texture.Width, texture.Height), 2) - 1);

            try
            {
                dds = DDSIO.GetDDSFile(texture);
            }
            catch
            {
                return texture;
            }

            File.WriteAllBytes(savePath, dds);

            string texConvFormat = "";
            if (formatOptimization)
            {
                if (texture.Format == TextureFormat.D3DFMT_DXT5 ||
                    texture.Format == TextureFormat.D3DFMT_A1R5G5B5 ||
                    texture.Format == TextureFormat.D3DFMT_A8B8G8R8 ||
                    texture.Format == TextureFormat.D3DFMT_A8R8G8B8)
                {
                    texConvFormat = "BC3_UNORM";
                }
                else
                {
                    texConvFormat = "BC1_UNORM";
                }
            }
            else
            {
                switch (texture.Format)
                {
                    // compressed
                    case TextureFormat.D3DFMT_DXT1: texConvFormat = "BC1_UNORM"; break;
                    case TextureFormat.D3DFMT_DXT3: texConvFormat = "BC2_UNORM"; break;
                    case TextureFormat.D3DFMT_DXT5: texConvFormat = "BC3_UNORM"; break;
                    case TextureFormat.D3DFMT_ATI1: texConvFormat = "BC4_UNORM"; break;
                    case TextureFormat.D3DFMT_ATI2: texConvFormat = "BC5_UNORM"; break;
                    case TextureFormat.D3DFMT_BC7: texConvFormat = "BC5_UNORM"; break;

                    // uncompressed
                    case TextureFormat.D3DFMT_A1R5G5B5: texConvFormat = "B5G5R5A1_UNORM"; break;
                    case TextureFormat.D3DFMT_A8: texConvFormat = "A8_UNORM"; break;
                    case TextureFormat.D3DFMT_A8B8G8R8: texConvFormat = "R8G8B8A8_UNORM"; break;
                    case TextureFormat.D3DFMT_L8: texConvFormat = "R8_UNORM"; break;
                    case TextureFormat.D3DFMT_A8R8G8B8: texConvFormat = "B8G8R8A8_UNORM"; break;
                }
            }

            if (downsize)
            {
                texture.Width /= 2;
                texture.Height /= 2;
                texture.Levels = Convert.ToByte(Math.Log(Math.Min(texture.Width, texture.Height), 2));
            }

            Process texConvertation = new();
            texConvertation.StartInfo.FileName = "Dependencies/texconv.exe";
            texConvertation.StartInfo.Arguments = $"-w {texture.Width} -h {texture.Height} -m {texture.Levels} temp.dds -y -f {texConvFormat}";
            texConvertation.StartInfo.UseShellExecute = false;
            texConvertation.StartInfo.CreateNoWindow = true;

            texConvertation.Start();

            texConvertation.WaitForExit();

            dds = File.ReadAllBytes("temp.dds");
            Texture tex = DDSIO.GetTexture(dds);

            texture.Data = tex.Data;
            texture.Width = tex.Width;
            texture.Height = tex.Height;
            texture.Depth = tex.Depth;
            texture.Levels = tex.Levels;
            texture.Format = tex.Format;
            texture.Stride = tex.Stride;

            return texture;
        }

        private static float flagToSize(int flag)
        {
            return (((flag >> 17) & 0x7f) + (((flag >> 11) & 0x3f) << 1) + (((flag >> 7) & 0xf) << 2) + (((flag >> 5) & 0x3) << 3) + (((flag >> 4) & 0x1) << 4)) * (0x2000 << (flag & 0xF));
        }

        private static float[] GetFileSize(string filePath)
        {
            FileStream fs = new(filePath, FileMode.Open);
            BinaryReader reader = new(fs);
            byte[] data = new byte[4];
            int virtualSize;
            int physicalSize;

            reader.Read(data, 0, 4);
            char[] magic = System.Text.Encoding.UTF8.GetString(data).ToCharArray();

            string magStr = new(magic);

            if (magStr != "RSC7")
            {
                fs.Close();
                return new float[] { 0, 0 };
            }

            reader.Read(data, 0, 4);
            _ = BitConverter.ToInt16(data);

            reader.Read(data, 0, 4);
            virtualSize = BitConverter.ToInt32(data);

            reader.Read(data, 0, 4);
            physicalSize = BitConverter.ToInt32(data);

            float vSize = flagToSize(virtualSize) / 1024 / 1024;
            float pSize = flagToSize(physicalSize) / 1024 / 1024;

            fs.Close();

            return new float[] { vSize, pSize };
        }

        private static RpfFileEntry CreateFileEntry(string name, string path, ref byte[] data)
        {
            uint rsc7 = (data?.Length > 4) ? BitConverter.ToUInt32(data, 0) : 0;
            //this should only really be used when loading a file from the filesystem.
            RpfFileEntry e;
            if (rsc7 == 0x37435352) //RSC7 header present! create RpfResourceFileEntry and decompress data...
            {
                e = RpfFile.CreateResourceFileEntry(ref data, 0);//"version" should be loadable from the header in the data..
                data = ResourceBuilder.Decompress(data);
            }
            else
            {
                RpfBinaryFileEntry be = new RpfBinaryFileEntry
                {
                    FileSize = (uint)data?.Length
                };
                be.FileUncompressedSize = be.FileSize;
                e = be;
            }
            e.Name = name;
            e.NameLower = name?.ToLowerInvariant();
            e.NameHash = JenkHash.GenHash(e.NameLower);
            e.ShortNameHash = JenkHash.GenHash(Path.GetFileNameWithoutExtension(e.NameLower));
            e.Path = path;
            return e;
        }

        private static YtdFile CreateYtdFile(string path)
        {
            byte[] data = File.ReadAllBytes(path);
            string name = new FileInfo(path).Name;

            RpfFileEntry fe = CreateFileEntry(name, path, ref data);

            YtdFile ytd = RpfFile.GetFile<YtdFile>(fe, data);

            return ytd;
        }

        public static ResultsData Optimize(string inputDirectory, string backupDirectory, string optimizeSize, bool onlyOverSized, bool downsize, bool formatOptimization, Delegate optimizeProgressHandler)
        {
            ResultsData resultsData = new();

            string[] inputFiles = Directory.GetFiles(inputDirectory, "*.ytd", SearchOption.AllDirectories);
            ushort optimizeSizeValue = Convert.ToUInt16(optimizeSize);
            bool doBackup = backupDirectory != "";

            int currentProgress = 0;

            for (int i = 0; i < inputFiles.Length; i++)
            {
                string filePath = inputFiles[i];
                string fileName = Path.GetFileName(filePath);

                float[] fileSizes = GetFileSize(filePath);

                if (onlyOverSized && fileSizes[1] < 16 || (fileSizes[0] == 0.0f && fileSizes[1] == 0.0f))
                {
                    continue;
                }

                YtdFile ytdFile = CreateYtdFile(filePath);

                bool ytdChanged = false;

                for (int j = 0; j < ytdFile.TextureDict.Textures.Count; j++)
                {
                    Texture texture = ytdFile.TextureDict.Textures[j];

                    if (onlyOverSized || (texture.Width + texture.Height) >= optimizeSizeValue)
                    {
                        if (!ytdChanged)
                        {
                            if (doBackup)
                            {
                                try
                                {
                                    string relativePath = Path.GetRelativePath(inputDirectory, filePath);
                                    string[] dirs = relativePath.Split('\\');
                                    string backupPath = backupDirectory;
                                    for (int k = 0; k < dirs.Length - 1; k++)
                                    {
                                        backupPath += "\\" + dirs[k];

                                        if (!System.IO.Directory.Exists(backupPath))
                                        {
                                            Directory.CreateDirectory(backupPath);
                                        }
                                    }

                                    File.Copy(filePath, backupPath + "\\" + fileName);
                                }
                                catch {}
                        }
                            ytdChanged = true;
                        }

                        Texture newTexture = OptimizeTexture(texture, formatOptimization, downsize);

                        resultsData.filesOptimized++;

                        ytdFile.TextureDict.Textures.data_items[j] = newTexture;
                    }
                }

                if (ytdChanged)
                {
                    byte[] newData = ytdFile.Save();
                    File.WriteAllBytes(filePath, newData);

                    float[] newFileSizes = GetFileSize(filePath);
                    resultsData.optimizedSize += fileSizes[1] - newFileSizes[1];
                }

                int progress = (i * 100 / inputFiles.Length);
                if (currentProgress != progress)
                {
                    optimizeProgressHandler?.DynamicInvoke(resultsData, progress);
                    currentProgress = progress;
                }
            }

            optimizeProgressHandler?.DynamicInvoke(resultsData, 100);

            return resultsData;
        }

        public static StatsData GetStatsData(string path, Delegate updateHandler)
        {
            StatsData results = new();
            string[] inputFiles = Directory.GetFiles(path, "*.ytd", SearchOption.AllDirectories);

            if (inputFiles.Length == 0)
            {
                return results;
            }

            results.filesCount = inputFiles.Length;

            int currentProgress = 0;

            for (int i = 0; i < inputFiles.Length; i++)
            {
                string filePath = inputFiles[i];

                float[] sizes = GetFileSize(filePath);

                results.virtualSize += sizes[0];
                results.physicalSize += sizes[1];

                if (sizes[1] > 16)
                {
                    results.oversizedCount++;
                }

                int progress = (i * 100 / inputFiles.Length);

                if (currentProgress != progress)
                {
                    updateHandler?.DynamicInvoke(progress);
                    currentProgress = progress;
                }
            }

            // 100%
            updateHandler?.DynamicInvoke(100);

            return results;
        }
    }
}
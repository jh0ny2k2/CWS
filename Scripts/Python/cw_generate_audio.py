import os
import math
import struct
import wave
import unreal

def generate_diesel_wav(filepath, duration_sec=2.0, sample_rate=22050):
    num_samples = int(duration_sec * sample_rate)
    f0 = 24.0
    f_cylinder = f0 * 3.0
    
    samples = []
    for i in range(num_samples):
        t = float(i) / sample_rate
        combustion = (math.sin(2.0 * math.pi * f_cylinder * t) +
                      0.5 * math.sin(4.0 * math.pi * f_cylinder * t + 0.3) +
                      0.25 * math.sin(6.0 * math.pi * f_cylinder * t + 0.7))
        sub = 0.4 * math.sin(2.0 * math.pi * f0 * t)
        noise = ((((i * 1103515245 + 12345) & 0x7FFFFFFF) / float(0x7FFFFFFF)) - 0.5) * 0.12
        
        val = (combustion * 0.6 + sub + noise) * 0.7
        val = max(-1.0, min(1.0, val))
        samples.append(int(val * 32767.0))
        
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        data = struct.pack('<' + ('h' * len(samples)), *samples)
        wav_file.writeframes(data)
    unreal.log("Generado wav diesel: " + filepath)


def generate_hydraulic_wav(filepath, duration_sec=1.5, sample_rate=22050):
    num_samples = int(duration_sec * sample_rate)
    f_pump = 360.0
    
    samples = []
    for i in range(num_samples):
        t = float(i) / sample_rate
        pump = (math.sin(2.0 * math.pi * f_pump * t) +
                0.3 * math.sin(4.0 * math.pi * f_pump * t) +
                0.15 * math.sin(6.0 * math.pi * f_pump * t))
        noise = ((((i * 1664525 + 1013904223) & 0x7FFFFFFF) / float(0x7FFFFFFF)) - 0.5) * 0.35
        
        val = (pump * 0.5 + noise * 0.5) * 0.6
        val = max(-1.0, min(1.0, val))
        samples.append(int(val * 32767.0))
        
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    with wave.open(filepath, 'w') as wav_file:
        wav_file.setnchannels(1)
        wav_file.setsampwidth(2)
        wav_file.setframerate(sample_rate)
        data = struct.pack('<' + ('h' * len(samples)), *samples)
        wav_file.writeframes(data)
    unreal.log("Generado wav hidraulica: " + filepath)


def import_sound(wav_path, dest_package, asset_name):
    abs_wav = os.path.abspath(wav_path)
    task = unreal.AssetImportTask()
    task.filename = abs_wav
    task.destination_path = dest_package
    task.destination_name = asset_name
    task.replace_existing = True
    task.automated = True
    task.save = True
    
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    sound = unreal.load_asset(dest_package + "/" + asset_name)
    if sound:
        sound.set_editor_property("looping", True)
        unreal.EditorAssetLibrary.save_loaded_asset(sound)
        unreal.log("Asset SoundWave importado y guardado en: " + dest_package + "/" + asset_name)


if __name__ == "__main__":
    content_dir = unreal.Paths.project_content_dir()
    audio_dir = os.path.join(content_dir, "ConstructionWorld", "Audio")
    
    wav_diesel = os.path.join(audio_dir, "SW_CW_DieselEngine.wav")
    wav_hydro = os.path.join(audio_dir, "SW_CW_HydraulicPump.wav")
    
    generate_diesel_wav(wav_diesel)
    generate_hydraulic_wav(wav_hydro)
    
    import_sound(wav_diesel, "/Game/ConstructionWorld/Audio", "SW_CW_DieselEngine")
    import_sound(wav_hydro, "/Game/ConstructionWorld/Audio", "SW_CW_HydraulicPump")

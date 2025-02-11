import soundfile as sf
import os
import random
from pathlib import Path

def slice_audio(input_file, output_dir, min_duration=3, max_duration=4):
    """
    Slice an audio file into chunks of random duration between min_duration and max_duration seconds.
    """
    # Load the audio file
    data, samplerate = sf.read(input_file)
    
    # Get the total duration in seconds
    total_duration = len(data) / samplerate
    
    # Initialize the start time
    current_time = 0
    chunk_number = 0
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Get the base filename without extension
    base_name = Path(input_file).stem
    
    while current_time < total_duration:
        # Generate random duration for this chunk
        chunk_duration = random.uniform(min_duration, max_duration)
        
        # Calculate end time
        end_time = min(current_time + chunk_duration, total_duration)
        
        # Convert times to sample indices
        start_sample = int(current_time * samplerate)
        end_sample = int(end_time * samplerate)
        
        # Extract the chunk
        chunk = data[start_sample:end_sample]
        
        # Generate output filename
        output_file = os.path.join(output_dir, f"{base_name}_chunk_{chunk_number:03d}.wav")
        
        # Save the chunk
        sf.write(output_file, chunk, samplerate)
        
        # Update for next iteration
        current_time = end_time
        chunk_number += 1
        
        print(f"Created chunk {chunk_number}: {output_file}")

if __name__ == "__main__":
    # Define input and output paths
    current_dir = os.path.dirname(os.path.abspath(__file__))
    input_file = os.path.join(current_dir, "earle-brown-three-pieces.mp3")
    output_dir = current_dir  # Same directory as input file
    
    # Slice the audio
    slice_audio(input_file, output_dir)
    print("Audio slicing complete!")

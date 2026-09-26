# Overview

The `sound.json` file defines sound resources, audio file paths, localized paths, and base volume configurations used for audio effects in the game.

The file is composed of a map, assigning a name to a Sound entry (which can be a single SoundObject or an array of SoundObjects):

```jsonc
{
	"sound.explosion": {
		"path": "sounds/explosion.wav",
		"volume": 80.0
	},
	"sound.voice.unit": [
		{
			"path": "locales/<locale>/voice_01.wav",
			"localized": false
		},
		{
			"path": "locales/<locale>/voice_02.wav",
			"volume": 90.0,
			"localized": false
		}
	]
}
```

# Fields

Every sound definition maps a unique sound identifier string to either a single `SoundObject` or an array of `SoundObject` entries (`SoundObject[]`).
When multiple sound objects are specified in an array, one is randomly selected to play each time the sound is triggered.

```jsonc
{
	"path": string,       // The relative file path to the audio file. Standard localized asset paths may include the "<locale>" tag, which is dynamically substituted at runtime.
	"volume": float?,     // The playback volume adjustment for this audio track.
	"localized": boolean? // Is this sound spatialized in the game world. (By default true) 
}
```

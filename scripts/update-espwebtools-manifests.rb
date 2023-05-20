require 'uri'
require 'json'

manifest = {
  name: ENV['MANIFEST_NAME'],
  version: ENV['MANIFEST_VERSION'],
  builds: []
}

if ENV['ESP8266_IMAGE_URI']
  manifest[:builds] << {
    chipFamily: 'ESP8266',
    parts: [
      { path: ENV['ESP8266_IMAGE_URI'], offset: 0 }
    ]
  }
end

if ENV['ESP32_IMAGE_URI']
  manifest[:builds] << {
    chipFamily: 'ESP32',
    parts: [
      { path: ENV['ESP32_IMAGE_URI'], offset: 0 }
    ]
  }
end

File.write("install-konnected/manifests/#{ENV['MANIFEST_FNAME']}", JSON.dump(manifest))
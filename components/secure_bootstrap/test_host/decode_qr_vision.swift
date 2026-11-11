import AppKit
import Vision

guard CommandLine.arguments.count == 2 else { fatalError("expected PNG path") }
let url = URL(fileURLWithPath: CommandLine.arguments[1])
guard let image = NSImage(contentsOf: url) else { fatalError("cannot load image") }
var rect = CGRect(origin: .zero, size: image.size)
guard let cgImage = image.cgImage(forProposedRect: &rect, context: nil, hints: nil) else {
    fatalError("cannot decode image pixels")
}
let request = VNDetectBarcodesRequest()
request.symbologies = [.qr]
try VNImageRequestHandler(cgImage: cgImage).perform([request])
let values = (request.results ?? []).compactMap { $0.payloadStringValue }
guard values.count == 1 else { fatalError("expected exactly one QR, got \(values.count)") }
print(values[0])

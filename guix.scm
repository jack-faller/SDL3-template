(use-modules (guix gexp)
						 (guix packages)
						 (guix git-download)
             (guix build-system cmake)
						 (nongnu packages nvidia)
						 ((guix licenses) #:prefix license:)
						 (gnu packages))

(define sd3-template
  (package
   (name "sd3-template")
   (version "0.0.0")
   (source (local-file (dirname (current-filename)) #:recursive? #t))
   (build-system cmake-build-system)
   (native-inputs (map specification->package '("glad")))
   (inputs (cons (specification->package "sdl3")
								 (map specification->package '())))
   (home-page "")
   (synopsis "")
   (description "")
   (license license:gpl3)))
sd3-template

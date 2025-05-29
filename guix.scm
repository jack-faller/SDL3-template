(use-modules (guix gexp)
			 (guix packages)
			 (guix git-download)
             (guix build-system cmake)
			 (nongnu packages nvidia)
			 ((guix licenses) #:prefix license:)
			 (gnu packages))

(define sd3-test
  (package
   (name "sd3-test")
   (version "0.0.0")
   (source (local-file (dirname (current-filename)) #:recursive? #t))
   (build-system cmake-build-system)
   (native-inputs (map specification->package '("glad")))
   (inputs (cons (replace-mesa (specification->package "sdl3"))
				 (map specification->package '())))
   (home-page "https://jackfaller.xyz")
   (synopsis "My website")
   (description "My website")
   (license license:gpl3)))
sd3-test
